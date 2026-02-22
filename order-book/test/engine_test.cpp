#include <gtest/gtest.h>

#include "engine.hpp"
#include "order.hpp"
#include "order_book.hpp"
#include "common/report.hpp"

template <typename T, typename Y>
void producer(T* report_mem, Y report)
{
    uint64_t local_write_idx = report_mem->write_idx;
    uint64_t cached_read_idx = report_mem->read_idx;
    while (local_write_idx - cached_read_idx >= engine::global::BUFFER_CAPACITY)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        __builtin_ia32_pause();
        cached_read_idx = report_mem->read_idx;
    }

    report_mem->buffer[local_write_idx % engine::global::BUFFER_CAPACITY] = report;
    std::atomic_thread_fence(std::memory_order_release);
    report_mem->write_idx = local_write_idx + 1;
}

TEST(mem_map_test, maps_memory)
{
    engine::Engine engine;
    const char* shm_path = "/dev/shm/test_mem"; 

    auto* prod_ptr = engine.mem_map<int>(shm_path, engine::mem::Mem_flags::PRODUCER);
    ASSERT_NE(prod_ptr, nullptr);

    auto* cons_ptr = engine.mem_map<int>(shm_path, engine::mem::Mem_flags::CONSUMER);
    ASSERT_NE(cons_ptr, nullptr);
}

TEST(rust_feeder, handles_rust_data)
{
    const char* shm_path = "/dev/shm/test_mem_rust_feeder"; 
    const char* report_path = "/dev/shm/test_mem_rust_report"; 

    engine::Engine engine;
    Order_book book;

    auto* prod_ptr = engine.mem_map<common::memory_struct<common::Order>>(shm_path, engine::mem::Mem_flags::PRODUCER);
    
    engine.rust_order = engine.mem_map<common::memory_struct<common::Order>>(shm_path, engine::mem::Mem_flags::CONSUMER);
    
    engine.report_mem = engine.mem_map<common::memory_struct<common::Report>>(report_path, engine::mem::Mem_flags::PRODUCER);

    {
        common::Order d = {123, 123, 123, 1, 1, 0}; 
        producer(prod_ptr, d);

        engine.rust_function(book);

        ASSERT_EQ(engine.capture_val, 3);
        ASSERT_EQ(engine.rust_order->read_idx, 1);
        
        auto lookup_it = book.order_lookup.find(d.id);
        ASSERT_EQ(lookup_it->second.location->ID, d.id);
        ASSERT_EQ(lookup_it->second.location->price, d.price);
        ASSERT_EQ(lookup_it->second.location->size, d.size);
        ASSERT_EQ(lookup_it->second.location->type, d.side);
    }

    {
        engine.capture_val = 0;
        common::Order d1 = {124, 123, 123, 1, 1, 1}; 
        producer(prod_ptr, d1);
        
        engine.rust_function(book);

        ASSERT_EQ(engine.capture_val, 1);
        ASSERT_EQ(engine.rust_order->read_idx, 2);
    }

    {
        engine.capture_val = 0;
        common::Order d = {123, 0, 0, 1, 2, 0}; 
        producer(prod_ptr, d);

        engine.rust_function(book);

        ASSERT_EQ(engine.capture_val, 2);
        ASSERT_EQ(engine.rust_order->read_idx, 3);

        auto lookup_it = book.order_lookup.find(d.id);
        ASSERT_EQ(lookup_it, book.order_lookup.end());
    }
}

TEST(strategy_order, handles_strategy_data)
{
    const char* shm_path = "/dev/shm/test_strategy_func"; 
    const char* report_path = "/dev/shm/test_strategy_reports";

    engine::Engine engine;
    Order_book book;

    auto* prod_ptr = engine.mem_map<common::memory_struct<common::Order>>(shm_path, engine::mem::Mem_flags::PRODUCER);
    engine.strategy_order = engine.mem_map<common::memory_struct<common::Order>>(shm_path, engine::mem::Mem_flags::CONSUMER);
    engine.report_mem = engine.mem_map<common::memory_struct<common::Report>>(report_path, engine::mem::Mem_flags::PRODUCER);

    {

        common::Order d = {123, 123, 123, 1, 1, 0}; 
        producer(prod_ptr, d);

        engine.strategy_order_func(book);

        ASSERT_EQ(engine.capture_val, 2);
        ASSERT_EQ(engine.strategy_order->read_idx, 1);
        
        auto lookup_it = book.order_lookup.find(d.id);
        ASSERT_EQ(lookup_it->second.location->ID, d.id);
        ASSERT_EQ(lookup_it->second.location->price, d.price);
        ASSERT_EQ(lookup_it->second.location->size, d.size);
        ASSERT_EQ(lookup_it->second.location->type, d.side);

    }

    {
        engine.capture_val = 0;
        common::Order d = {123, 0, 0, 1, 2, 0}; 
        producer(prod_ptr, d);

        engine.strategy_order_func(book);

        ASSERT_EQ(engine.capture_val, 1);
        ASSERT_EQ(engine.strategy_order->read_idx, 2);

        auto lookup_it = book.order_lookup.find(d.id);
        ASSERT_EQ(lookup_it, book.order_lookup.end());
    }
}


TEST(send_report, report_function)
{
    const char* report_path = "/dev/shm/test_send_report";

    engine::Engine engine;
    Order_book book;

    engine.report_mem = engine.mem_map<common::memory_struct<common::Report>>(report_path, engine::mem::Mem_flags::PRODUCER);
    auto consumer = engine.mem_map<common::memory_struct<common::Report>>(report_path, engine::mem::Mem_flags::CONSUMER);
    common::Report rep = common::Report{123, common::rep::Status::NEW, 123, 123 ,123 , common::Order_side::BUY, common::rep::Rejection_code::NOERROR, 123 ,123 };
    engine.send_report(engine.report_mem, rep);

    ASSERT_EQ(consumer->read_idx, engine.report_mem->read_idx);
    ASSERT_EQ(consumer->write_idx, engine.report_mem->write_idx);
    ASSERT_EQ(consumer->write_idx, 1); 
    ASSERT_EQ(consumer->read_idx, 0); 

    ASSERT_EQ(consumer->buffer->order_id, rep.order_id);
    ASSERT_EQ(consumer->buffer->status, rep.status);
    ASSERT_EQ(consumer->buffer->last_quantity, rep.last_quantity);
    ASSERT_EQ(consumer->buffer->last_price, rep.last_price);
    ASSERT_EQ(consumer->buffer->side, rep.side);
    ASSERT_EQ(consumer->buffer->reject_code, rep.reject_code);
    ASSERT_EQ(consumer->buffer->trade_id, rep.trade_id);
    ASSERT_EQ(consumer->buffer->timestamp, rep.timestamp);
}

TEST(send_candle, candle_function)
{
    const char* candle_path = "/dev/shm/test_send_candles";
    engine::Engine engine;
    Order_book book;

    engine.candle_mem = engine.mem_map<common::memory_struct<common::Candle>>(candle_path, engine::mem::Mem_flags::PRODUCER);
    auto consumer = engine.mem_map<common::memory_struct<common::Candle>>(candle_path, engine::mem::Mem_flags::CONSUMER);

    common::Candle can = common::Candle{123, 123, 123, 123, 123};

    engine.send_candle(engine.candle_mem, can);

    ASSERT_EQ(consumer->write_idx, engine.candle_mem->write_idx); 
    ASSERT_EQ(consumer->read_idx, engine.candle_mem->read_idx); 
    ASSERT_EQ(consumer->write_idx, 1); 
    ASSERT_EQ(consumer->read_idx, 0); 

    ASSERT_EQ(consumer->buffer[0].close, can.close);
    ASSERT_EQ(consumer->buffer[0].high, can.high);
    ASSERT_EQ(consumer->buffer[0].low, can.low);
    ASSERT_EQ(consumer->buffer[0].open, can.open);
    ASSERT_EQ(consumer->buffer[0].volume, can.volume);
}

