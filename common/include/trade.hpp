#pragma once
#include <cstddef>
#include <cstdint>


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
    long price;
    Order_type type;
};


