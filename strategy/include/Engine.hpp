#pragma once
#include "candle.hpp"
#include "order.hpp"
#include "portfolio.hpp"
#include "report.hpp"
#include "ring_buffer.hpp"
#include "dashboard.hpp"
#include <chrono>
#include <cstdint>
#include "time.hpp"
#include <deque>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef UNIT_TEST
class report_function_repo_Test;
#endif

namespace backtester
{
    static constexpr int BUFFER_CAPACITY = 16384;

    template <typename T> struct memory_struct
    {
        volatile uint64_t write_idx;
        uint8_t           pad1[56];
        volatile uint64_t read_idx;
        uint8_t           pad2[56];
        T                 buffer[16384];
    };

    enum class MemoryFlags
    {
        PRODUCER,
        CONSUMER,
    };

    template <typename Strategy> 
    class Engine
    {

#ifdef UNIT_TEST
    friend class ::report_function_repo_Test;
#endif 

    public:
        Engine(double starting_cash, double fees)
            : portfolio(starting_cash, fees)
        {
            ring_buffer = backtester::Ring_buffer();
            history.reserve(1000);
        }
        template <typename T> 
        T* map_mem(const char* path, MemoryFlags flag)
        {
            int fd = 0;

            if (flag == MemoryFlags::CONSUMER)
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
            else if (flag == MemoryFlags::PRODUCER)
            {
                shm_unlink(path);

                fd = open(path, O_RDWR | O_CREAT, 0666);
                if (fd == -1)
                {
                    std::cout << ("open failed");
                    return nullptr;
                }
                if (ftruncate(fd, sizeof(T)) == -1)
                {
                    std::cout << ("ftruncate failed");
                    return nullptr;
                }
            }

            void* ptr = mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

            if (fd != -1)
                close(fd);

            if (ptr == MAP_FAILED)
            {
                perror("mmap failed");
                return nullptr;
            }

            return static_cast<T*>(ptr);
        }

        void connect()
        {
            order_mem  = map_mem<memory_struct<Order>>("/dev/shm/hft_order", MemoryFlags::PRODUCER);
            candle_mem = map_mem<memory_struct<Candle>>("/dev/shm/hft_candle", MemoryFlags::CONSUMER);
            report_mem = map_mem<memory_struct<Report>>("/dev/shm/hft_report", MemoryFlags::CONSUMER);
            dashboard_mem = map_mem<Dashboard_state>("/dev/shm/hft_dashboard", MemoryFlags::PRODUCER);

            if (dashboard_mem) 
            {
                dashboard_mem->cash = portfolio.starting_cash;
                dashboard_mem->active_order_count = 0;
            }
        }

        void run()
        {
            uint64_t local_read_idx = candle_mem->write_idx;
            candle_mem->read_idx    = local_read_idx;

            uint64_t local_report_read_idx = report_mem->write_idx;
            report_mem->read_idx           = local_report_read_idx;

            while (true)
            {
                write_order();
                uint64_t current_write_idx        = candle_mem->write_idx;
                uint64_t current_report_write_idx = report_mem->write_idx;

                if (local_read_idx < current_write_idx)
                {
                    int    slot = local_read_idx % 16384;
                    Candle raw  = candle_mem->buffer[slot];

                    ring_buffer.add(raw);
                    local_read_idx++;
                    candle_mem->read_idx = local_read_idx;

                    if (warm_count > 0)
                    {
                        warm_count--;
                        continue;
                    }
                    double price = ring_buffer.get(0).get_open();

                    if (price < 0.0001)
                    { 
                        local_read_idx++;
                        candle_mem->read_idx = local_read_idx;
                        continue;
                    }
                    strategy.run(ring_buffer, *this);
                }
                if (local_report_read_idx < current_report_write_idx)
                {
                    int    slot = local_report_read_idx % 16384;
                    Report raw  = report_mem->buffer[slot];

                    on_report(raw);

                    local_report_read_idx++;
                    report_mem->read_idx = local_report_read_idx;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        }

        void set_delay(uint64_t time)
        {
            this->delay = time;
        }

        void order(float size, float price, Order_side side)
        {
            if (side == Order_side::BUY)
            {
                if ((price * size) > portfolio.get_cash())
                {
                    return; 
                }
            }
            else if (side == Order_side::SELL)
            {
                if (size > portfolio.get_position())
                {
                    return;
                }
            }
            int8_t   int_side   = (side == Order_side::BUY) ? 0 : 1;
            uint64_t p          = price * 100;
            uint64_t s          = size * 1000000;
            my_order_ids.insert(order_id);
            Order    order           = Order(order_id++, s, p, int_side, 0, 0 /** status*/);
            delayed_orders.emplace_back(order, get_real_time_ms() + delay);
        }

        void cancel_order(uint64_t target_id)
        {
            Order    order = Order(target_id, 0 /**SIZE*/, 0 /**PRICE*/, 0 /**SIDE*/, 1 /**ACTION*/, 0 /**Status*/);
            uint64_t local_write_idx = order_mem->write_idx;
            uint64_t cached_read_idx = order_mem->read_idx;
            if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY)
            {
                cached_read_idx = order_mem->read_idx;
            }
            if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY)
            {
                return;
            }
            order_mem->buffer[local_write_idx % BUFFER_CAPACITY] = order;
            std::atomic_thread_fence(std::memory_order_release);
            order_mem->write_idx = local_write_idx + 1;
        }

        void set_warmup(size_t size)
        {
            warm_count = size;
        }

        backtester::Ring_buffer* get_ring_buffer()
        {
            return &ring_buffer;
        }

        void on_report(const Report& raw)
        {
            if (my_order_ids.find(raw.order_id) == my_order_ids.end())
            {
                return;
            }

            if (raw.order_id == 0 && raw.last_price == 0 && raw.leaves_quantity == 0)
            {
                return;
            }

            if (raw.status != Status::NEW)
                history.push_back(raw);

            if (raw.status == Status::NEW)
            {
                portfolio.update(raw, (double)raw.last_price);
                active_orders[raw.order_id]
                    = Active_orders { raw.order_id, raw.leaves_quantity, raw.last_price, raw.side, raw.timestamp };
            }
            else
            {
                auto it = active_orders.find(raw.order_id);
                if (it != active_orders.end())
                {
                    double original_limit = (double)it->second.price;
                    portfolio.update(raw, original_limit);

                    if (raw.status == Status::FILLED || raw.status == Status::CANCELED)
                    {
                        active_orders.erase(it);
                        my_order_ids.erase(raw.order_id);
                    }
                    else if (raw.status == Status::PARTIALLY_FILLED)
                    {
                        it->second.leaves_quantity = raw.leaves_quantity;
                    }
                }
                else
                {
                    if (raw.status == Status::FILLED || raw.status == Status::PARTIALLY_FILLED)
                    {
                        portfolio.update(raw, (double)raw.last_price);

                        if (raw.status == Status::FILLED)
                        {
                            my_order_ids.erase(raw.order_id);
                        }
                    }
                }
            }

            if (dashboard_mem)
            {
                dashboard_mem->cash = portfolio.cash;
                dashboard_mem->locked_cash = portfolio.locked_cash;
                dashboard_mem->position = portfolio.position;
                dashboard_mem->total_fees = portfolio.total_fees;
                dashboard_mem->active_order_count = active_orders.size();
                dashboard_mem->last_update_ts = raw.timestamp;

                if (raw.status == Status::FILLED)
                {
                    dashboard_mem->total_trades+=1;
                    dashboard_mem->last_trade_price = (double)raw.last_price / 100.0;
                    dashboard_mem->last_trade_id = raw.order_id;
                }
            }
        }

    private:

        void write_order()  
        {
            if (delayed_orders.empty())
            {
                return;
            }
            if (get_real_time_ms() >= delayed_orders[0].time)
            {
                uint64_t local_write_idx = order_mem->write_idx;
                uint64_t cached_read_idx = order_mem->read_idx;
                while (local_write_idx - cached_read_idx >= BUFFER_CAPACITY)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    cached_read_idx = order_mem->read_idx;
                }
                order_mem->buffer[local_write_idx % BUFFER_CAPACITY] = delayed_orders[0].order;
                std::atomic_thread_fence(std::memory_order_release);
                order_mem->write_idx = local_write_idx + 1;
                delayed_orders.pop_front();
            }
        }

        Strategy               strategy;
        memory_struct<Candle>* candle_mem;
        memory_struct<Order>*  order_mem;
        memory_struct<Report>* report_mem;
        Ring_buffer            ring_buffer;
        size_t                 warm_count = 0;
        uint64_t               order_id   = 0;

        Portfolio portfolio;

        Dashboard_state* dashboard_mem;

        std::vector<Report>                         history;
        std::unordered_map<uint64_t, Active_orders> active_orders;
        std::unordered_set<uint64_t>                my_order_ids;

        std::deque<Delayed_order> delayed_orders;
        uint64_t delay;

    };
}; // namespace backtester
