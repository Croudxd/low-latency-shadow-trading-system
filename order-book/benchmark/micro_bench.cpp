#include "order.hpp"
#include "order_book.hpp"
#include <benchmark/benchmark.h>

auto null_sender = [](const Rep::Report& rep) { benchmark::DoNotOptimize(rep); };

static void BM_OrderAdd_NoMatch(benchmark::State& state)
{
    Order_book book;
    uint64_t   id = 1;

    for (auto _ : state)
    {
        state.PauseTiming();
        Order ord(Order_type::buy, 100, 10, id++);
        state.ResumeTiming();

        book.add_order(ord, Flags::NONMATCH, null_sender);
    }
}

static void BM_OrderMatch(benchmark::State& state)
{
    Order_book book;

    for (int i = 0; i < 1000; ++i)
    {
        Order sell(Order_type::sell, 100, 10, i + 1);
        book.add_order(sell, Flags::NONMATCH, null_sender);
    }

    uint64_t buy_id = 2000;

    for (auto _ : state)
    {
        state.PauseTiming();
        Order buy(Order_type::buy, 100, 10, buy_id++);
        state.ResumeTiming();

        book.add_order(buy, Flags::MATCH, null_sender);
    }
    state.PauseTiming();
}

static void BM_OrderMatch_Refill(benchmark::State& state)
{

    for (auto _ : state)
    {
        state.PauseTiming();
        Order_book book;
        for (int i = 0; i < 1000; ++i)
        {
            Order sell(Order_type::sell, 100, 10, i + 1);
            book.add_order(sell, Flags::NONMATCH, null_sender);
        }
        Order buy(Order_type::buy, 100, 10000, 9999); 
        state.ResumeTiming();

        book.add_order(buy, Flags::MATCH, null_sender);
    }
}
BENCHMARK(BM_OrderMatch_Refill);
BENCHMARK(BM_OrderAdd_NoMatch);
BENCHMARK(BM_OrderMatch);

BENCHMARK_MAIN();
