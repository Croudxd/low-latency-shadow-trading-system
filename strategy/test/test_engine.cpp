#include <gtest/gtest.h>
#include <atomic>
#include "Engine.hpp"
#include "common/candle.hpp"
#include "indicator.hpp"
#include "common/report.hpp"
#include "ring_buffer.hpp"
#include "sma.hpp"

class Strategy
{

    public:
        void run(strategy::Ring_buffer& ring_buffer, strategy::Engine<Strategy>& engine)
        {
        }
};

template <typename T, typename Y>
void producer(T order_mem, Y order)
{
            uint64_t local_write_idx = order_mem->write_idx;
            uint64_t cached_read_idx = order_mem->read_idx;
            if (local_write_idx - cached_read_idx >= strategy::BUFFER_CAPACITY)
            {
                cached_read_idx = order_mem->read_idx;
            }
            if (local_write_idx - cached_read_idx >= strategy::BUFFER_CAPACITY)
            {
                return;
            }
            order_mem->buffer[local_write_idx % strategy::BUFFER_CAPACITY] = order;
            std::atomic_thread_fence(std::memory_order_release);
            order_mem->write_idx = local_write_idx + 1;
}

TEST(mem_map_test, maps_memory)
{
    const char* path = "/dev/shm/test_mem_map"; 
    strategy::Engine<Strategy> engine(100000000.0, 0.000001);
    auto prod = engine.map_mem<common::memory_struct<int>>(path, strategy::MemoryFlags::PRODUCER);
    producer(prod, 123123);
    auto consum = engine.map_mem<common::memory_struct<int>>(path, strategy::MemoryFlags::CONSUMER);

    uint64_t local_read_idx = consum->write_idx;
    consum->read_idx    = local_read_idx;
    uint64_t current_write_idx = consum->write_idx;

    if (local_read_idx < current_write_idx)
    {
        int slot = local_read_idx % 16384;
        int raw  = consum->buffer[slot];
        ASSERT_EQ(raw, 123123);
    }
}

TEST(order, send_order)
{
    const char* path = "/dev/shm/test_order"; 
    strategy::Engine<Strategy> engine(100000000.0, 0.000001);
    common::Order ord = common::Order(123, 123, 123, 1, 0, 1);
    auto prod = engine.map_mem<common::memory_struct<common::Order>>(path, strategy::MemoryFlags::PRODUCER);
    producer(prod, ord);
    auto consum = engine.map_mem<common::memory_struct<common::Order>>(path, strategy::MemoryFlags::CONSUMER);

    uint64_t local_read_idx = consum->write_idx;
    consum->read_idx    = local_read_idx;
    uint64_t current_write_idx = consum->write_idx;

    if (local_read_idx < current_write_idx)
    {
        int slot = local_read_idx % 16384;
        common::Order raw  = consum->buffer[slot];
        ASSERT_EQ(raw.id, 123);
        ASSERT_EQ(raw.price, 123);
        ASSERT_EQ(raw.size, 123);
        ASSERT_EQ(raw.side, 1);
        ASSERT_EQ(raw.status, 0);
        ASSERT_EQ(raw.action, 1);
    }
}

TEST(cancel_order, cancel_order)
{

    const char* path = "/dev/shm/test_order"; 
    strategy::Engine<Strategy> engine(100000000.0, 0.000001);
    common::Order ord = common::Order(123, 0, 0, 0, 1, 0);
    auto prod = engine.map_mem<common::memory_struct<common::Order>>(path, strategy::MemoryFlags::PRODUCER);
    producer(prod, ord);
    auto consum = engine.map_mem<common::memory_struct<common::Order>>(path, strategy::MemoryFlags::CONSUMER);

    uint64_t local_read_idx = consum->write_idx;
    consum->read_idx    = local_read_idx;
    uint64_t current_write_idx = consum->write_idx;

    if (local_read_idx < current_write_idx)
    {
        int slot = local_read_idx % 16384;
        common::Order raw  = consum->buffer[slot];
        ASSERT_EQ(raw.id, 123);
        ASSERT_EQ(raw.price, 0);
        ASSERT_EQ(raw.size, 0);
        ASSERT_EQ(raw.side, 0);
        ASSERT_EQ(raw.status, 1);
        ASSERT_EQ(raw.action, 0);
    }
}

TEST(report_function, repo)
{
    strategy::Engine<Strategy> engine(100000000.0, 0.0);
    engine.my_order_ids.insert(1001);

    common::Report new_rep(1001, common::rep::Status::NEW, 0, 10000, 1000000, common::Order_side::BUY, common::rep::Rejection_code::NOERROR, 0, 100);
    engine.on_report(new_rep);

    double limit_price = 10000.0 / 100.0;
    double total_qty   = 1000000.0 / 1000000.0;
    double locked      = limit_price * total_qty;
    double expected    = 100000000.0 - locked;

    ASSERT_NEAR(engine.portfolio.get_cash(), expected, 1e-7);
    // printf("NEW Cash: %.8f\n", engine.portfolio.get_cash());

    common::Report part_rep(1001, common::rep::Status::PARTIALLY_FILLED, 500000, 9900, 500000, common::Order_side::BUY, common::rep::Rejection_code::NOERROR, 1, 101);
    engine.on_report(part_rep);

    double part_price = 9900.0 / 100.0;
    double part_qty   = 500000.0 / 1000000.0;
    double trade_val  = part_price * part_qty;
    double lock_ref   = limit_price * part_qty;
    expected += (lock_ref - trade_val);

    ASSERT_NEAR(engine.portfolio.get_cash(), expected, 1e-7);
    // printf("PART Cash: %.8f\n", engine.portfolio.get_cash());

    common::Report fill_rep(1001, common::rep::Status::FILLED, 500000, 10000, 0, common::Order_side::BUY, common::rep::Rejection_code::NOERROR, 2, 102);
    engine.on_report(fill_rep);

    double fill_price = 10000.0 / 100.0;
    double fill_qty   = 500000.0 / 1000000.0;
    trade_val = fill_price * fill_qty;
    lock_ref  = limit_price * fill_qty;
    expected += (lock_ref - trade_val);

    ASSERT_NEAR(engine.portfolio.get_cash(), expected, 1e-7);
    // printf("FILL Cash: %.8f\n", engine.portfolio.get_cash());
}


TEST(SMA, sma_calc)
{
    common::Candle candle = common::Candle(1, 1, 1, 1, 1);
    common::Candle candle1 = common::Candle(1, 1, 1, 1, 1);
    common::Candle candle2 = common::Candle(1, 1, 1, 1, 1);
    strategy::Ring_buffer rn = strategy::Ring_buffer();
    auto x = strategy::Indicator();
    x.set_ring_buffer(&rn);
    ASSERT_EQ(strategy::SMA(3).calculate(), 0) ;

    rn.add(candle);
    rn.add(candle1);
    rn.add(candle2);
    auto y = strategy::SMA(3).calculate();
    ASSERT_EQ(y, 1) ;

    common::Candle candle3 = common::Candle(112312, 11231, 198998, 8939, 991723);
    common::Candle candle4 = common::Candle(987824, 987432, 987934, 987234, 123978);
    common::Candle candle5 = common::Candle(987493, 98375, 948738, 346773, 875634);

    rn.add(candle3);
    rn.add(candle4);
    rn.add(candle5);

    auto z = strategy::SMA(5).calculate();
    ASSERT_NEAR(z, 417526.2, 0.0001);


    auto v = strategy::SMA(6).calculate();
    ASSERT_NEAR(v, 347938.666, 0.001);
}

TEST(ring_buf, data_structure)
{
    common::Candle candle3 = common::Candle(112312, 11231, 198998, 8939, 991723);
    common::Candle candle4 = common::Candle(987824, 987432, 987934, 987234, 123978);
    common::Candle candle5 = common::Candle(987493, 98375, 948738, 346773, 875634);
    strategy::Ring_buffer rn = strategy::Ring_buffer();
    rn.add(candle3);
    ASSERT_EQ(rn.size(), 1);
    ASSERT_EQ(rn.get(0).get_open(), candle3.get_open());
}
