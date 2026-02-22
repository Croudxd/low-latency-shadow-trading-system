#pragma once
#include "memory.hpp"
#include <common/spsc_memory_struct.hpp>
#include <common/report.hpp>
#include <common/candle.hpp>
#include <common/trade.hpp>
#include <common/order.hpp>

namespace network
{
    class Sender
    {
        private:
            common::memory_struct<common::Report>* report_mem;
            common::memory_struct<common::Candle>* candle_mem;
            common::memory_struct<common::Trade>* trade_mem;
            common::memory_struct<common::Order>* order_mem;

        public:
            void connect()
            {
                common::Report rep;
                memory::Memory mem = memory::Memory();
                report_mem = mem.mem_map<common::memory_struct<common::Report>>("/path/", memory::Mem_flags::CONSUMER);
                candle_mem = mem.mem_map<common::memory_struct<common::Candle>>("/path/", memory::Mem_flags::CONSUMER);
                trade_mem = mem.mem_map<common::memory_struct<common::Trade>>("/path/", memory::Mem_flags::CONSUMER);
                order_mem = mem.mem_map<common::memory_struct<common::Order>>("/path/", memory::Mem_flags::CONSUMER);
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
                        send_to_kdb<Y>(raw);
                    }
                    else
                    {
                        std::this_thread::yield();
                    }
                }
            }

            template <typename Y>
            void send_to_kdb (Y raw)
            {

            }
    };
}

