#pragma once
#include <cstdint>
namespace common
{
    template <typename T> struct memory_struct
    {
        volatile uint64_t write_idx;
        uint8_t           pad1[56];
        volatile uint64_t read_idx;
        uint8_t           pad2[56];
        T                 buffer[16384];
    };
}
