#pragma once
#include "memory.hpp"
#include <common/spsc_memory_struct.hpp>
#include <common/report.hpp>
#include <common/candle.hpp>
#include <common/trade.hpp>
#include <common/order.hpp>
#include "q/k.h"

namespace network
{
    class Sender
    {
        private:
            common::memory_struct<common::Report>* report_mem;
            common::memory_struct<common::Candle>* candle_mem;
            common::memory_struct<common::Trade>* trade_mem;
            common::memory_struct<common::Order>* order_mem;
            int handle;

        public:
            void connect()
            {

                handle = khpu((char*)"localhost", 5001, (char*)"");
                if (handle <= 0) 
                {
                    std::cerr << "Failed to connect! Is kdb+ running on port 5001?" << std::endl;
                }

                std::cout << "Connected successfully! Handle ID: " << handle << std::endl;
                common::Report rep;
                memory::Memory mem = memory::Memory();
                report_mem = mem.mem_map<common::memory_struct<common::Report>>("/dev/shm/hft_send_report", memory::Mem_flags::CONSUMER);
                order_mem = mem.mem_map<common::memory_struct<common::Order>>("/dev/shm/hft_send_order", memory::Mem_flags::CONSUMER);
                trade_mem = mem.mem_map<common::memory_struct<common::Trade>>("/dev/shm/hft_send_trade", memory::Mem_flags::CONSUMER);
                candle_mem = mem.mem_map<common::memory_struct<common::Candle>>("/dev/shm/hft_send_candle", memory::Mem_flags::CONSUMER);
            }
            
            template <typename T, typename Y>
            T read_spsc(T* mem_lay)
            {
                uint64_t local_read_idx = mem_lay->write_idx;
                mem_lay->read_idx    = local_read_idx;

                while (true)
                {
                    uint64_t current_write_idx = mem_lay->write_idx;

                    if (local_read_idx < current_write_idx)
                    {

                        std::atomic_thread_fence(std::memory_order_acquire);
                        int slot = local_read_idx % 16384;
                        Y raw  = mem_lay->buffer[slot];

                        send_to_kdb<Y>(raw);
                        local_read_idx++;
                        std::atomic_thread_fence(std::memory_order_release);
                        mem_lay->read_idx = local_read_idx;
                    }
                    else
                    {
                        std::this_thread::yield();
                    }
                }
            }

            void close_kdb()
            {
                kclose(handle);
            }

            K pack_for_kdb(const common::Candle& raw) {
                return knk(5, kj(raw.open), kj(raw.high), kj(raw.low), kj(raw.close), kj(raw.volume));
            }

            K pack_for_kdb(const common::Order& raw) {
                return knk(6, kj(raw.id), kj(raw.size), kj(raw.price), kg(raw.side), kg(raw.action), kg(raw.status));
            }

            K pack_for_kdb(const common::Report& raw) {
                return knk(9, kj(raw.order_id), kj(raw.last_quantity), kj(raw.last_price), kj(raw.leaves_quantity), kj(raw.trade_id), kj(raw.timestamp), kg(raw.status), kg(raw.side), kg(raw.reject_code));
            }

            K pack_for_kdb(const common::Trade& raw) {
                return knk(4, kj(raw.time), kj(raw.size), kj(raw.price), kj(raw.type));
            }

            const char* get_table_name(const common::Candle&) { return "candle"; }
            const char* get_table_name(const common::Order&)  { return "order"; }
            const char* get_table_name(const common::Report&) { return "report"; }
            const char* get_table_name(const common::Trade&)  { return "trade"; }

            template <typename Y>
            void send_to_kdb (Y raw)
            {
                K data_list = pack_for_kdb(raw);
                K result = k(-handle, (char*)"insert", ks((char*)get_table_name(raw)), data_list, (K)0);

                if (!result) 
                {
                    std::cerr << "Network error during communication." << std::endl;
                } 
                else if (result->t == -128) 
                { 
                    r0(result);
                }
                else 
                {
                    r0(result); 
                }

            }

            void run()
            {
                while (true)
                {
                    read_spsc<common::memory_struct<common::Report>, common::Report>(report_mem);
                    read_spsc<common::memory_struct<common::Candle>, common::Candle>(candle_mem);
                    read_spsc<common::memory_struct<common::Trade>, common::Trade>(trade_mem);
                    read_spsc<common::memory_struct<common::Order>, common::Order>(order_mem);
                }
            }
    };
}

