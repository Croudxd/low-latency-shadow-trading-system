#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <atomic>
#include <sched.h>
#include <pthread.h>
#include <immintrin.h> // For _mm_pause

#include "engine.hpp" 
#include "report.hpp"
#include "order_book.hpp"

using namespace engine;

const int NUM_ORDERS = 100'000'000; 
const int PRICE_MIN = 95;
const int PRICE_MAX = 105;

template <typename T>
T* setup_shm_producer(const char* path) {
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    ftruncate(fd, sizeof(T));
    void* ptr = mmap(NULL, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    auto layout = static_cast<T*>(ptr);
    layout->write_idx = 0;
    layout->read_idx = 0;
    return layout;
}

int main() {
    // --- 1. PREPARE DATA ---
    std::vector<mem::Data> test_data;
    test_data.reserve(NUM_ORDERS);
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> type_dist(0, 1);
    std::uniform_int_distribution<int> price_dist(PRICE_MIN, PRICE_MAX);
    std::uniform_int_distribution<int> size_dist(1, 100);

    for (int i = 0; i < NUM_ORDERS; ++i) {
        test_data.push_back({(uint64_t)i, (uint64_t)size_dist(rng), (int32_t)price_dist(rng), (int8_t)type_dist(rng), 0, 0, {0}});
    }

    // --- TEST A: RAW ENGINE SPEED (No IPC) ---
    {
        std::cout << "Starting Test A: Raw Engine (Direct Loop)..." << std::endl;
        Order_book raw_book;
        auto sender = [](const Rep::Report& rep) { /* No-op for raw test */ };
        
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto& d : test_data) {
            Order_type type = (d.side == 0) ? Order_type::buy : Order_type::sell;
            Order ord{type, d.price, (size_t)d.size, d.id};
            raw_book.add_order(std::move(ord), Flags::MATCH, sender);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        std::cout << "RAW ENGINE - Latency: " << (ns / NUM_ORDERS) << " ns/order" << std::endl;
    }

    // --- TEST B: IPC ENGINE SPEED ---
    {
        std::cout << "\nStarting Test B: IPC Engine (Pinned + SHM)..." << std::endl;
        auto strategy_shm = setup_shm_producer<mem::memory_layout<mem::Data>>("/dev/shm/hft_order");
        setup_shm_producer<mem::memory_layout<Candle>>("/dev/shm/hft_candle");
        setup_shm_producer<mem::memory_layout<Rep::Report>>("/dev/shm/hft_report");

        Engine engine_inst;
        std::atomic<bool> keep_running{true};
        std::thread engine_thread([&]() {
            engine_inst.connect();
            Order_book book;
            while (keep_running.load(std::memory_order_relaxed)) {
                engine_inst.strategy_order_func(book);
            }
        });

        // Pinning logic
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset); CPU_SET(1, &cpuset); 
        pthread_setaffinity_np(engine_thread.native_handle(), sizeof(cpu_set_t), &cpuset);
        CPU_ZERO(&cpuset); CPU_SET(2, &cpuset); 
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < NUM_ORDERS; ++i) {
            uint64_t next_write = strategy_shm->write_idx;
            // HIGH PERF SPIN: Replace yield with pause
            while (next_write - strategy_shm->read_idx >= global::BUFFER_CAPACITY) {
                _mm_pause(); 
            }
            strategy_shm->buffer[next_write % global::BUFFER_CAPACITY] = test_data[i];
            std::atomic_thread_fence(std::memory_order_release);
            strategy_shm->write_idx = next_write + 1;
        }

        while (strategy_shm->read_idx < (uint64_t)NUM_ORDERS) { _mm_pause(); }
        auto end = std::chrono::high_resolution_clock::now();

        keep_running = false;
        engine_thread.join();

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        std::cout << "IPC ENGINE - Latency: " << (ns / NUM_ORDERS) << " ns/order" << std::endl;
    }

    return 0;
}
