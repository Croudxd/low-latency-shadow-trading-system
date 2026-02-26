#pragma once
#include "common/utils.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <atomic>
#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <immintrin.h> // For _mm_pause()

#include <common/report.hpp>
#include <common/trade.hpp>
#include <common/candle.hpp>
#include <common/spsc_memory_struct.hpp>
#include <common/order.hpp>

namespace memory
{
    namespace global
    {
        static constexpr int BUFFER_CAPACITY = 16384;
    }

    enum class Mem_flags
    {
        CONSUMER,
        PRODUCER,
    };

    class Memory
    {
        private:
            common::memory_struct<common::Report>* report_mem = nullptr;
            common::memory_struct<common::Order>* order_mem = nullptr;
            common::memory_struct<common::Candle>* candle_mem = nullptr;
            // Removed trade_mem consumer since trades are generated from filled orders

            common::memory_struct<common::Report>* report_mem_prod = nullptr;
            common::memory_struct<common::Order>* order_mem_prod = nullptr;
            common::memory_struct<common::Trade>* trade_mem_prod = nullptr;
            common::memory_struct<common::Candle>* candle_mem_prod = nullptr;

        public:
            template <typename T> 
            T* mem_map(const char* path, Mem_flags flag)
            {
                int fd = 0;
                if (flag == Mem_flags::CONSUMER)
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
                else if (flag == Mem_flags::PRODUCER)
                {
                    shm_unlink(path); 
                    fd = open(path, O_RDWR | O_CREAT, 0666);
                    if (fd == -1) { std::cout << ("open failed\n"); return nullptr; }
                    if (ftruncate(fd, sizeof(T)) == -1) { std::cout << ("ftruncate failed\n"); return nullptr; }
                }

                void* ptr = mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                close(fd); 

                if (ptr == MAP_FAILED) return nullptr;
                return static_cast<T*>(ptr);
            }

            void connect()
            {
                report_mem = mem_map<common::memory_struct<common::Report>>("/dev/shm/hft_report", Mem_flags::CONSUMER);
                order_mem = mem_map<common::memory_struct<common::Order>>("/dev/shm/hft_ring", Mem_flags::CONSUMER);
                candle_mem = mem_map<common::memory_struct<common::Candle>>("/dev/shm/hft_candle", Mem_flags::CONSUMER);

                report_mem_prod = mem_map<common::memory_struct<common::Report>>("/dev/shm/hft_send_report", Mem_flags::PRODUCER);
                order_mem_prod = mem_map<common::memory_struct<common::Order>>("/dev/shm/hft_send_order", Mem_flags::PRODUCER);
                trade_mem_prod = mem_map<common::memory_struct<common::Trade>>("/dev/shm/hft_send_trade", Mem_flags::PRODUCER);
                candle_mem_prod = mem_map<common::memory_struct<common::Candle>>("/dev/shm/hft_send_candle", Mem_flags::PRODUCER);
            }

            template <typename T, typename Y>
            void write_spsc(T* mem_lay_prod, const Y& mem)
            {
                if (!mem_lay_prod) return;
                uint64_t local_write_idx = mem_lay_prod->write_idx;
                uint64_t cached_read_idx = mem_lay_prod->read_idx;

                if (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY) return; // Drop if consumer is too slow

                mem_lay_prod->buffer[local_write_idx % global::BUFFER_CAPACITY] = mem;
                std::atomic_thread_fence(std::memory_order_release);
                mem_lay_prod->write_idx = local_write_idx + 1;
            }

            template <typename T, typename Y>
            void poll_spsc(T* mem_lay, T* mem_lay_prod)
            {
                if (!mem_lay || !mem_lay_prod) return;
                uint64_t local_read_idx = mem_lay->read_idx;
                uint64_t current_write_idx = mem_lay->write_idx;

                while (local_read_idx < current_write_idx)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    int slot = local_read_idx % global::BUFFER_CAPACITY;
                    const Y& raw = mem_lay->buffer[slot];
                    
                    write_spsc(mem_lay_prod, raw);
                    
                    local_read_idx++;
                    std::atomic_thread_fence(std::memory_order_release);
                    mem_lay->read_idx = local_read_idx;
                }
            }

            // Specialized poller for Orders to derive Trades
            void poll_order_spsc()
            {
                if (!order_mem) return;
                uint64_t local_read_idx = order_mem->read_idx;
                uint64_t current_write_idx = order_mem->write_idx;

                while (local_read_idx < current_write_idx)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    int slot = local_read_idx % global::BUFFER_CAPACITY;
                    const common::Order& raw = order_mem->buffer[slot];

                    if (raw.status == 1) // 1 == FILLED
                    {
                        common::Trade tr = {cstime::get_timestamp(), raw.size, raw.price, 
                            (raw.side == common::Order_side::BUY) ? common::Order_type::buy : common::Order_type::sell}; 
                        
                        write_spsc(trade_mem_prod, tr); // Write to the actual producer queue!
                    }
                    else 
                    {
                        write_spsc(order_mem_prod, raw);
                    }

                    local_read_idx++;
                    std::atomic_thread_fence(std::memory_order_release);
                    order_mem->read_idx = local_read_idx;
                }
            }

            void run()
            {
                while (true)
                {
                    poll_spsc<common::memory_struct<common::Candle>, common::Candle>(candle_mem, candle_mem_prod);
                    poll_spsc<common::memory_struct<common::Report>, common::Report>(report_mem, report_mem_prod);
                    poll_order_spsc(); // Handles orders and generates trades
                    
                    _mm_pause(); // Keep CPU happy
                }
            }
    };
}
