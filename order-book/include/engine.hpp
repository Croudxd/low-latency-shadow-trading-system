#pragma once

#include "common/order.hpp"
#include "utils.hpp"
#include <common/candle.hpp>
#include <order.hpp>
#include <order_book.hpp>
#include <common/spsc_memory_struct.hpp>
#include <common/report.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <atomic>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

#ifdef UNIT_TEST
class rust_feeder_handles_rust_data_Test;
class send_report_report_function_Test;
class strategy_order_handles_strategy_data_Test;
class add_order_check_book_Test;
class send_candle_candle_function_Test;
#endif

namespace engine
{
    namespace global
    {
        static constexpr int BUCKET_SIZE     = 1 * 100000;
        static constexpr int BUFFER_CAPACITY = 16384;
    } // namespace global

    namespace mem
    {
        enum class Mem_flags
        {
            CONSUMER,
            PRODUCER,
        };
    } // namespace mem
      //
    enum engine_flags 
    {
        TEST,
        NORMAL,
    };

    class Engine
    {

    /** testing */
        #ifdef UNIT_TEST
        friend class ::rust_feeder_handles_rust_data_Test;
        friend class ::send_report_report_function_Test;
        friend class ::strategy_order_handles_strategy_data_Test;
        friend class ::send_candle_candle_function_Test;
        friend class ::add_order_check_book_Test;
        int capture_val = 0;
        #endif


    public:
        Engine() = default;

        template <typename T> T* mem_map(const char* path, mem::Mem_flags flag)
        {
            int fd = 0;
            if (flag == mem::Mem_flags::CONSUMER)
            {
                while (true)
                {
                    fd = open(path, O_RDWR);
                    if (fd != -1)
                    {
                        struct stat st;
                        if (fstat(fd, &st) == 0 && st.st_size >= static_cast<off_t>(sizeof(T)))
                        {
                            break; 
                        }
                        close(fd); 
                    }
                    sleep(1);
                }
            }
            else if (flag == mem::Mem_flags::PRODUCER)
            {
                shm_unlink(path); 
                fd = open(path, O_RDWR | O_CREAT, 0666);
                if (fd == -1) { std::cout << ("open failed"); return nullptr; }
                if (ftruncate(fd, sizeof(T)) == -1) { std::cout << ("ftruncate failed"); return nullptr; }
            }

            void* ptr = mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            close(fd); 

            if (ptr == MAP_FAILED)
            {
                perror("mmap failed");
                return nullptr;
            }
            return static_cast<T*>(ptr);
        }

        void send_candle(common::memory_struct<common::Candle>* candle_shm, common::Candle candle)
        {
            uint64_t local_write_idx = candle_shm->write_idx;
            uint64_t cached_read_idx = candle_shm->read_idx;
            while (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                __builtin_ia32_pause();
                cached_read_idx = candle_shm->read_idx;
            }

            candle_shm->buffer[local_write_idx % global::BUFFER_CAPACITY] = candle;
            std::atomic_thread_fence(std::memory_order_release);
            candle_shm->write_idx = local_write_idx + 1;
        }

        void send_report(common::memory_struct<common::Report>* report_mem, common::Report report)
        {
            uint64_t local_write_idx = report_mem->write_idx;
            uint64_t cached_read_idx = report_mem->read_idx;
            while (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                __builtin_ia32_pause();
                cached_read_idx = report_mem->read_idx;
            }

            report_mem->buffer[local_write_idx % global::BUFFER_CAPACITY] = report;
            std::atomic_thread_fence(std::memory_order_release);
            report_mem->write_idx = local_write_idx + 1;
        }

        void connect()
        {
            candle_mem     = mem_map<common::memory_struct<common::Candle>>("/dev/shm/hft_candle", mem::Mem_flags::PRODUCER);
            report_mem     = mem_map<common::memory_struct<common::Report>>("/dev/shm/hft_report", mem::Mem_flags::PRODUCER);

            rust_order     = mem_map<common::memory_struct<common::Order>>("/dev/shm/hft_ring", mem::Mem_flags::CONSUMER);
            strategy_order = mem_map<common::memory_struct<common::Order>>("/dev/shm/hft_order", mem::Mem_flags::CONSUMER);



            rust_local_read_idx     = rust_order->write_idx;
            strategy_local_read_idx = strategy_order->write_idx;
        }

        void strategy_order_func(Order_book& book)
        {
            uint64_t current_write_idx = strategy_order->write_idx;

            auto sender = [&](const common::Report& rep) 
            {
                this->send_report(this->report_mem, rep);
            };

            while (strategy_local_read_idx < current_write_idx)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                int       slot = strategy_local_read_idx % global::BUFFER_CAPACITY;
                common::Order raw  = strategy_order->buffer[slot];
                auto      side = (raw.side == 0) ? Order_type::buy : Order_type::sell;
                Order     ord  = { side, raw.price, raw.size, raw.id };
                if (raw.action == 2)
                {

                    book.cancel_order(raw.id, sender);
                    #ifdef UNIT_TEST
                    capture_val = 1;
                    #endif
                }
                else
                {
                    book.add_order(ord, Flags::MATCH, sender);
                    #ifdef UNIT_TEST
                    capture_val = 2;
                    #endif
                }

                strategy_local_read_idx++;
            }
            strategy_order->read_idx = strategy_local_read_idx;
        }

        void rust_function(Order_book& book)
        {
            uint64_t current_write_idx = rust_order->write_idx;

            auto sender = [&](const common::Report& rep) 
            {
                this->send_report(this->report_mem, rep);
            };
            while (rust_local_read_idx < current_write_idx)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                int       slot = rust_local_read_idx % global::BUFFER_CAPACITY;
                common::Order raw  = rust_order->buffer[slot];
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
                        common::Candle candle
                            = common::Candle { current_open, current_high, current_low, close, current_local_bucket_size, cstime::get_timestamp()};
                        send_candle(candle_mem, candle);
                        current_local_bucket_size = 0;
                        current_open              = 0;
                        current_high              = 0;
                        current_low               = std::numeric_limits<long>::max();
                        candle.print();
                    }
                    #ifdef UNIT_TEST
                    this->capture_val = 1;
                    #endif
                    auto taker_side = (raw.side == 0) ? Order_type::buy : Order_type::sell;

                    Order dummy_taker(taker_side, raw.price, raw.size, 0);
                    book.add_order(dummy_taker, Flags::MATCH, [](const common::Report& rep){ });
                }
                else if (raw.action == 2)
                {
                    book.cancel_order(raw.id, sender);

                    #ifdef UNIT_TEST
                    this->capture_val = 2;
                    #endif
                }
                else
                {
                    #ifdef UNIT_TEST
                    this->capture_val = 3;
                    #endif
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
        common::memory_struct<common::Order>* rust_order;
        common::memory_struct<common::Order>* strategy_order;
        common::memory_struct<common::Candle>*    candle_mem;
        common::memory_struct<common::Report>* report_mem;

        uint64_t rust_local_read_idx     = 0;
        uint64_t strategy_local_read_idx = 0;

        long current_local_bucket_size = 0;
        long current_open              = 0;
        long current_high              = 0;
        long current_low               = std::numeric_limits<long>::max();
    };
} // namespace engine
