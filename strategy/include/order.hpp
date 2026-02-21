#pragma once
#include <iostream>


#include <cstdint>
#include <sys/types.h>
namespace backtester
{
    enum class Order_side
    {
        BUY,
        SELL,
    };

    struct Order 
    {
        uint64_t id;
        uint64_t size;
        int64_t  price;
        int8_t   side;   //sell / buy
        int8_t   action; // cancel order 
        int8_t   status; // trade/order
        uint8_t  pad1[1];

        Order() = default;

        Order(uint64_t id, uint64_t size, int64_t price, int8_t side, int8_t action, int8_t status) : id(id), size(size), price(price), side(side), action(action), status(status)
        {
            pad1[0] = 0;
        }

     };

    struct Delayed_order
    {
        Order order;
        uint64_t time;
    };
}

