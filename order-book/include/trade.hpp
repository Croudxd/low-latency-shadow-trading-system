#pragma once
#include "order.hpp"
#include <cstddef>
#include <cstdint>

struct Trade 
{
    uint64_t time;
    size_t size;
    long price;
    Order_type type;
};


