#pragma once 
#include <cstdint>
#include <chrono>

uint64_t get_real_time_ms() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}
