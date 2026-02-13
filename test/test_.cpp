#include <iostream>
#include <vector>
#include <chrono>
#include "engine.hpp" // For engine::mem::Data and memory_layout

using namespace engine::mem;

// Simple fast RNG for HFT benchmarking
struct FastRNG {
    uint64_t state = 42;
    uint32_t next() {
        state = state * 6364136223846793005ULL + 1;
        return (uint32_t)(state >> 32);
    }
};

int main() {
    // 1. Setup Shared Memory
    engine::Engine helper;
    auto* rust_ring = helper.mem_map<memory_layout<Data>>("/dev/shm/hft_ring", Mem_flags::PRODUCER);
    auto* strat_ring = helper.mem_map<memory_layout<Data>>("/dev/shm/hft_order", Mem_flags::PRODUCER);

    rust_ring->write_idx = 0;
    rust_ring->read_idx = 0;
    strat_ring->write_idx = 0;
    strat_ring->read_idx = 0;

    std::cout << "FIREHOSE INITIALIZED. ARMING BOMBS..." << std::endl;
    FastRNG rng;
    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t total_sent = 0;

    while (true) {
        // --- STREAM 1: THE RUST TRADE FIREHOSE ---
        // We generate 100 trades at a time to reduce atomic overhead
        for (int i = 0; i < 100; ++i) {
            uint64_t head = rust_ring->write_idx;
            
            // Check for buffer overflow
            if (head - rust_ring->read_idx >= engine::global::BUFFER_CAPACITY) break;

            Data trade;
            trade.id = total_sent++;
            trade.price = 50000 + (rng.next() % 1000); // BTC-style price
            trade.size = 1 + (rng.next() % 10);
            trade.side = (rng.next() % 2);
            trade.action = 0;
            trade.status = 1; // STATUS 1 = RAW TRADE (Triggers Candle Aggregation)

            rust_ring->buffer[head % engine::global::BUFFER_CAPACITY] = trade;
            std::atomic_thread_fence(std::memory_order_release);
            rust_ring->write_idx = head + 1;
        }

        // --- STREAM 2: THE STRATEGY ORDER BURST ---
        // Occasionally dump massive limit orders to stress the Order Book maps
        if (total_sent % 500 == 0) {
            uint64_t head = strat_ring->write_idx;
            if (head - strat_ring->read_idx < engine::global::BUFFER_CAPACITY) {
                Data ord;
                ord.id = 1000000 + total_sent;
                ord.price = 50000 + (rng.next() % 1000);
                ord.size = 100;
                ord.side = (rng.next() % 2);
                ord.action = 0; // NEW ORDER
                ord.status = 0; 

                strat_ring->buffer[head % engine::global::BUFFER_CAPACITY] = ord;
                std::atomic_thread_fence(std::memory_order_release);
                strat_ring->write_idx = head + 1;
            }
        }

        // --- STATS LOGGING ---
        if (total_sent % 1000000 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            if (duration > 0) {
                std::cout << "Throughput: " << (total_sent / duration) / 1000000.0 
                          << " Million messages/sec" << std::endl;
            }
        }
    }

    return 0;
}
