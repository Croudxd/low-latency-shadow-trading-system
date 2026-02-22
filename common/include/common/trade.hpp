#pragma once
#include <cstddef>
#include <cstdint>


namespace common
{
    enum Order_type
    {
        buy,
        sell,
        null
    };

    struct Trade 
    {
        uint64_t time;
        size_t size;
        int64_t price;
        Order_type type;
    };
}


