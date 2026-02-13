#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

// Include your headers
#include "engine.hpp"
#include "order_book.hpp"

// ==========================================
// TEST UTILS: Shared Memory Manager
// ==========================================
// This helper class creates the /dev/shm files needed by Engine
// so the test doesn't hang waiting for them.
struct ShmEnvironment {
    std::vector<std::string> files = {
        "/dev/shm/hft_ring", 
        "/dev/shm/hft_order", 
        "/dev/shm/hft_candle", 
        "/dev/shm/hft_report"
    };

    ShmEnvironment() {
        for (const auto& path : files) {
            // Create/Truncate file
            int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
            if (fd == -1) {
                perror(("Failed to create " + path).c_str());
                exit(1);
            }
            // Size needs to be enough for the structs
            // Using a safe large size (1MB) to cover Engine::global::BUFFER_CAPACITY
            if (ftruncate(fd, 1024 * 1024) == -1) {
                perror("ftruncate failed");
            }
            close(fd);
        }
    }

    ~ShmEnvironment() {
        // Cleanup files after test
        for (const auto& path : files) {
            unlink(path.c_str());
        }
    }
};

// ==========================================
// REPORT CAPTURE
// ==========================================
struct ReportCapture {
    std::vector<Rep::Report> reports;
    void callback(const Rep::Report& r) {
        reports.push_back(r);
    }
    Rep::Report last() { return reports.back(); }
    void clear() { reports.clear(); }
};

// ==========================================
// TESTS
// ==========================================

void test_order_book_logic() {
    std::cout << "[TEST] Order Book Logic... ";
    
    Order_book book;
    ReportCapture cap;
    auto cb = [&](const Rep::Report& r){ cap.callback(r); };

    // 1. Add Buy Order
    // NOTE: ID 100. If your move-bug is active, report might show ID 0.
    book.add_order(Order(Order_type::buy, 100, 10, 100), Flags::MATCH, cb);
    
    // Check Report
    if (cap.reports.empty()) {
        std::cout << "FAIL (No Report Generated)" << std::endl;
        return;
    }
    
    Rep::Report r = cap.last();
    if (r.order_id == 0) {
        std::cout << "FAIL (Bug Detected: Order ID is 0. Fix std::move usage!)" << std::endl;
        return;
    }
    if (r.order_id != 100) {
        std::cout << "FAIL (Wrong ID: " << r.order_id << ")" << std::endl;
        return;
    }

    // 2. Match Logic
    book.add_order(Order(Order_type::sell, 100, 10, 101), Flags::MATCH, cb);
    
    auto trades = book.get_trade_history();
    if (trades.size() != 1) {
        std::cout << "FAIL (Trade not created)" << std::endl;
        return;
    }
    if (trades[0].price != 100 || trades[0].size != 10) {
        std::cout << "FAIL (Wrong Trade Data)" << std::endl;
        return;
    }

    std::cout << "PASS" << std::endl;
}

void test_engine_integration() {
    std::cout << "[TEST] Engine Integration... ";
    
    // 1. Setup Environment
    ShmEnvironment env; 
    engine::Engine eng;
    
    // 2. Connect (Should not hang now)
    eng.connect();

    // 3. Manually inject data into Shared Memory (simulating Rust Producer)
    // We map the file manually to write to it
    int fd = open("/dev/shm/hft_order", O_RDWR);
    auto* strat_mem = (engine::mem::memory_layout<engine::mem::Data>*)mmap(NULL, sizeof(engine::mem::memory_layout<engine::mem::Data>), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // Write an Order to the Ring Buffer
    uint64_t write_idx = strat_mem->write_idx; // Should be 0 initially
    int slot = write_idx % engine::global::BUFFER_CAPACITY;
    
    strat_mem->buffer[slot].id = 999;
    strat_mem->buffer[slot].price = 50;
    strat_mem->buffer[slot].size = 100;
    strat_mem->buffer[slot].side = 0; // Buy
    strat_mem->buffer[slot].action = 0; // New Order
    strat_mem->buffer[slot].status = 0; // Order
    
    // Commit write
    strat_mem->write_idx = write_idx + 1;
    
    // 4. Tick the Engine manually (instead of infinite run() loop)
    Order_book book;
    eng.strategy_order_func(book); // This should read from SHM and update book

    // 5. Verify Book State
    // We can't inspect book directly, but we can verify it accepted the order
    // by trying to match against it.
    ReportCapture cap;
    book.add_order(Order(Order_type::sell, 50, 100, 1000), Flags::MATCH, [&](const Rep::Report& r){ cap.callback(r); });

    auto trades = book.get_trade_history();
    if (trades.empty()) {
        std::cout << "FAIL (Engine did not process the order from Shared Memory)" << std::endl;
    } else {
        std::cout << "PASS" << std::endl;
    }
    
    munmap(strat_mem, sizeof(engine::mem::memory_layout<engine::mem::Data>));
    close(fd);
}

int main() {
    std::cout << "=== STARTING FULL SYSTEM TEST ===" << std::endl;
    
    try {
        test_order_book_logic();
        test_engine_integration();
    } catch (const std::exception& e) {
        std::cout << "CRITICAL EXCEPTION: " << e.what() << std::endl;
    }

    std::cout << "=== FINISHED ===" << std::endl;
    return 0;
}
