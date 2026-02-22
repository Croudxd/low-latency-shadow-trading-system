#pragma once
#include <iostream>
#include <cstdint>
#include <sys/types.h>
namespace common 
{
    enum Order_side
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

    struct Active_orders 
    {
        uint64_t order_id;
        uint64_t leaves_quantity;
        uint64_t price;
        Order_side side;
        uint64_t timestamp;

        void print() const
        {
            std::cout << "order_id" << order_id <<std::endl;
            std::cout << "order_id" << order_id <<std::endl;
            std::cout << "leaves_quantity" << leaves_quantity<<std::endl;
            std::cout << "price" << price <<std::endl;
        }
    };
}

