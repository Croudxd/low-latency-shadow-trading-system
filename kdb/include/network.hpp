#pragma once
#include "memory.hpp"

namespace network
{
    class Sender
    {
        private:
            memory::memory_layout<memory::Report>* report_mem;
            memory::memory_layout<memory::Candle>* candle_mem;
            memory::memory_layout<memory::Trade>* trade_mem;
            memory::memory_layout<memory::Order>* order_mem;

        public:
            void connect()
            {
                memory::Report rep;
                memory::Memory mem = memory::Memory();
                report_mem = mem.mem_map<memory::memory_layout<memory::Report>>("/path/", memory::Mem_flags::CONSUMER);
                candle_mem = mem.mem_map<memory::memory_layout<memory::Candle>>("/path/", memory::Mem_flags::CONSUMER);
                trade_mem = mem.mem_map<memory::memory_layout<memory::Trade>>("/path/", memory::Mem_flags::CONSUMER);
                order_mem = mem.mem_map<memory::memory_layout<memory::Order>>("/path/", memory::Mem_flags::CONSUMER);
            }
            
            template <typename T, typename Y>
            T read_spsc(T* mem_lay, T* mem_lay_prod)
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
                        // Send to a producer.
                    }
                    else
                    {
                        std::this_thread::yield();
                    }
                }
     
            }
    };
}

