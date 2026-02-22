#pragma once
#include <sys/stat.h>
#include <sys/types.h>
#include <atomic>
#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

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
            common::memory_struct<common::Report>* report_mem ;
            common::memory_struct<common::Order>* order_mem ;
            common::memory_struct<common::Trade>* trade_mem ;
            common::memory_struct<common::Candle>* candle_mem ;

            common::memory_struct<common::Report>* report_mem_prod;
            common::memory_struct<common::Order>* order_mem_prod;
            common::memory_struct<common::Trade>* trade_mem_prod;
            common::memory_struct<common::Candle>* candle_mem_prod;

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
                    if (fd == -1) { std::cout << ("open failed"); return nullptr; }
                    if (ftruncate(fd, sizeof(T)) == -1) { std::cout << ("ftruncate failed"); return nullptr; }
                }

                void* ptr = mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                close(fd); 

                if (ptr == MAP_FAILED)
                {
                    return nullptr;
                }
                return static_cast<T*>(ptr);
            }

            void connect()
            {
                report_mem = mem_map<common::memory_struct<common::Report>>("/dev/shm/hft_order", Mem_flags::CONSUMER);
                order_mem = mem_map<common::memory_struct<common::Order>>("/dev/shm/hft_order", Mem_flags::CONSUMER);
                trade_mem = mem_map<common::memory_struct<common::Trade>>("/dev/shm/hft_order", Mem_flags::CONSUMER);
                candle_mem = mem_map<common::memory_struct<common::Candle>>("/dev/shm/hft_order", Mem_flags::CONSUMER);

                report_mem_prod = mem_map<common::memory_struct<common::Report>>("/dev/shm/hft_order", Mem_flags::PRODUCER);
                order_mem_prod = mem_map<common::memory_struct<common::Order>>("/dev/shm/hft_order", Mem_flags::PRODUCER);
                trade_mem_prod = mem_map<common::memory_struct<common::Trade>>("/dev/shm/hft_order", Mem_flags::PRODUCER);
                candle_mem_prod = mem_map<common::memory_struct<common::Candle>>("/dev/shm/hft_order", Mem_flags::PRODUCER);
            }

            template <typename T, typename Y>
            void read_spsc(T* mem_lay, T* mem_lay_prod)
            {
                uint64_t local_read_idx = mem_lay->write_idx;
                mem_lay->read_idx    = local_read_idx;

                while (true)
                {
                    uint64_t current_write_idx = mem_lay->write_idx;

                    if (local_read_idx < current_write_idx)
                    {
                        int slot = local_read_idx % 16384;
                        Y raw  = mem_lay->buffer[slot];
                        write_spsc(mem_lay_prod, raw);
                    }
                    else
                    {
                        std::this_thread::yield();
                    }
                }
            }

            template <typename T, typename Y>
            void write_spsc(T* mem_lay_prod, Y mem)
            {
                uint64_t local_write_idx = mem_lay_prod->write_idx;
                uint64_t cached_read_idx = mem_lay_prod->read_idx;

                while (local_write_idx - cached_read_idx >= global::BUFFER_CAPACITY)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    __builtin_ia32_pause();
                    cached_read_idx = mem_lay_prod->read_idx;
                }

                mem_lay_prod->buffer[local_write_idx % global::BUFFER_CAPACITY] = mem;
                std::atomic_thread_fence(std::memory_order_release);
                mem_lay_prod->write_idx = local_write_idx + 1;
            }

            void run()
            {
                while (true)
                {
                    read_spsc<common::memory_struct<common::Candle>, common::Candle>(candle_mem, candle_mem_prod);
                    read_spsc<common::memory_struct<common::Report>, common::Report>(report_mem, report_mem_prod);
                    read_spsc<common::memory_struct<common::Order>, common::Order>(order_mem, order_mem_prod);
                    read_spsc<common::memory_struct<common::Trade>, common::Trade>(trade_mem, trade_mem_prod);
                }
            }
    };
}
