#pragma once

#include <atomic>
#include <candle.hpp>
#include <cstdint>
#include <fcntl.h>
#include <order.hpp>
#include <order_book.hpp>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <report.hpp>

namespace engine
{
    namespace global
    {
        static constexpr int BUCKET_SIZE     = 1 * 100000;
        static constexpr int BUFFER_CAPACITY = 16384;
    } // namespace global

    namespace mem
    {
        struct Data
        {
            uint64_t id;
            uint64_t size;
            int32_t  price;
            int8_t   side; //sell / buy
            int8_t   action; // cancel order
            int8_t   status; // trade/order
            uint8_t  pad1[1];
        };

        template <typename T> struct memory_layout
        {
            volatile uint64_t write_idx;
            uint8_t           pad1[56];
            volatile uint64_t read_idx;
            uint8_t           pad2[56];
            T                 buffer[global::BUFFER_CAPACITY];
        };

        enum class Mem_flags
        {
            CONSUMER,
            PRODUCER,
        };
    } // namespace mem

    class Engine
    {

    public:
        Engine() = default;

        template <typename T> T* mem_map(const char* path, mem::Mem_flags flag)
        {
            int fd = 0;
            if (flag == mem::Mem_flags::CONSUMER)
            {
                fd = open(path, O_RDWR);
                while (fd == -1)
                {
                    sleep(1);
                    fd = open(path, O_RDWR);
                }
            }
            if (flag == mem::Mem_flags::PRODUCER)
            {
                fd = open(path, O_RDWR | O_CREAT, 0666);
                ftruncate(fd, sizeof(T));
            }

            void* ptr = mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (ptr == MAP_FAILED)
            {
                perror("mmap failed");
                return nullptr;
            }
            std::cout << "Connected to memory" << std::endl;
            return static_cast<T*>(ptr);
        }

        void send_candle(mem::memory_layout<Candle>* candle_shm, Candle candle)
        {
            uint64_t local_write_idx = candle_shm->write_idx;
            uint64_t cached_read_idx = candle_shm->read_idx;
            if (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY)
            {
                cached_read_idx = candle_shm->read_idx;
            }
            if (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY)
            {
                return;
            }

            candle_shm->buffer[local_write_idx % global::BUFFER_CAPACITY] = candle;
            std::atomic_thread_fence(std::memory_order_release);
            candle_shm->write_idx = local_write_idx + 1;
        }

        void send_report(mem::memory_layout<Rep::Report>* report_mem, Rep::Report report)
        {
            uint64_t local_write_idx = report_mem->write_idx;
            uint64_t cached_read_idx = report_mem->read_idx;
            if (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY)
            {
                cached_read_idx = report_mem->read_idx;
            }
            if (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY)
            {
                return;
            }

            report_mem->buffer[local_write_idx % global::BUFFER_CAPACITY] = report;
            std::atomic_thread_fence(std::memory_order_release);
            report_mem->write_idx = local_write_idx + 1;
        }

        void connect()
        {
            rust_order     = mem_map<mem::memory_layout<mem::Data>>("/dev/shm/hft_ring", mem::Mem_flags::CONSUMER);
            strategy_order = mem_map<mem::memory_layout<mem::Data>>("/dev/shm/hft_order", mem::Mem_flags::CONSUMER);

            candle_mem     = mem_map<mem::memory_layout<Candle>>("/dev/shm/hft_candle", mem::Mem_flags::PRODUCER);
            report_mem     = mem_map<mem::memory_layout<Rep::Report>>("/dev/shm/hft_report", mem::Mem_flags::PRODUCER);

            rust_local_read_idx     = rust_order->write_idx;
            strategy_local_read_idx = strategy_order->write_idx;
        }

        void strategy_order_func(Order_book& book)
        {
            uint64_t current_write_idx = strategy_order->write_idx;

            auto sender = [&](const Rep::Report& rep) 
            {
                this->send_report(this->report_mem, rep);
            };

            if (strategy_local_read_idx < current_write_idx)
            {
                int       slot = strategy_local_read_idx % global::BUFFER_CAPACITY;
                mem::Data raw  = strategy_order->buffer[slot];
                auto      side = (raw.side == 0) ? Order_type::buy : Order_type::sell;
                Order     ord  = { side, raw.price, raw.size, raw.id };

                if (raw.action == 1)
                {
                    book.cancel_order(raw.id, sender);
                }
                else
                {
                    book.add_order(ord, Flags::MATCH, sender);
                }

                strategy_local_read_idx++;
            }
            strategy_order->read_idx = strategy_local_read_idx;
        }

        void rust_function(Order_book& book)
        {
            uint64_t current_write_idx = rust_order->write_idx;

            auto sender = [&](const Rep::Report& rep) 
            {
                this->send_report(this->report_mem, rep);
            };
            if (rust_local_read_idx < current_write_idx)
            {
                int       slot = rust_local_read_idx % global::BUFFER_CAPACITY;
                mem::Data raw  = rust_order->buffer[slot];
                auto      side = (raw.side == 0) ? Order_type::buy : Order_type::sell;
                Order     ord  = { side, raw.price, raw.size, raw.id };
                /** status = 1 is a trade not an order so we dont add to the book.*/
                if (raw.status == 1)
                {
                    if (current_open == 0)
                        current_open = raw.price;
                    if (raw.price > current_high)
                        current_high = raw.price;
                    if (raw.price < current_low)
                        current_low = raw.price;

                    current_local_bucket_size += raw.size;

                    if (current_local_bucket_size >= global::BUCKET_SIZE)
                    {
                        long   close = raw.price;
                        Candle candle
                            = Candle { current_open, current_high, current_low, close, current_local_bucket_size };
                        candle.print();
                        send_candle(candle_mem, candle);
                        current_local_bucket_size = 0;
                        current_open              = 0;
                        current_high              = 0;
                        current_low               = std::numeric_limits<long>::max();
                    }
                }
                else if (raw.action == 2)
                {
                    book.cancel_order(raw.id, sender);
                }
                else
                {
                    book.add_order(ord, Flags::NONMATCH, sender);
                }
                rust_local_read_idx++;
            }
            rust_order->read_idx = rust_local_read_idx;
        }

        void run()
        {
            Order_book book = Order_book {};
            while (true)
            {
                strategy_order_func(book);
                rust_function(book);
            }
        }

    private:
        mem::memory_layout<mem::Data>* rust_order;
        mem::memory_layout<mem::Data>* strategy_order;
        mem::memory_layout<Candle>*    candle_mem;
        mem::memory_layout<Rep::Report>* report_mem;

        uint64_t rust_local_read_idx     = 0;
        uint64_t strategy_local_read_idx = 0;

        long current_local_bucket_size = 0;
        long current_open              = 0;
        long current_high              = 0;
        long current_low               = std::numeric_limits<long>::max();
    };
} // namespace engine
