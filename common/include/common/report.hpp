#pragma once
#include "order.hpp"
#include <cstdint>
#include <iostream>

namespace common 
{
    namespace rep
    {
        enum Status 
        {
            NEW,
            FILLED,
            PARTIALLY_FILLED,
            CANCELED,
            REJECTED,
        };

        enum Rejection_code
        {
            NOERROR,
            NO_FUNDS,
            PRICE_OUT_OF_BOUNDS,
            SYSTEM_ERROR,
        };
    }

    struct Report 
    {
        uint64_t order_id;
        uint64_t last_quantity;
        uint64_t last_price;
        uint64_t leaves_quantity;
        uint64_t trade_id;
        uint64_t timestamp;

        rep::Status status;
        Order_side side;
        rep::Rejection_code reject_code;

        Report() = default;
        Report(uint64_t id, rep::Status stat, uint64_t l_qty, uint64_t l_px, 
               uint64_t leaves, Order_side s, rep::Rejection_code rej, 
               uint64_t t_id, uint64_t ts)
            : order_id(id), last_quantity(l_qty), last_price(l_px), 
              leaves_quantity(leaves), trade_id(t_id), timestamp(ts),
              status(stat), side(s), reject_code(rej) {}

        void print() const
        {
            std::string s = (status == rep::Status::CANCELED) ? "cancelled" : "fuck knows";
            std::string str = (side == Order_side::BUY) ? "buy" : "sell";

            std::cout << "order_id" << order_id <<std::endl;
            std::cout << "order_id" << order_id <<std::endl;
            std::cout << "last_quantity" << last_quantity<< std::endl;
            std::cout << "last_price" << last_price<<std::endl;
            std::cout << "leaves_quantity" << leaves_quantity<<std::endl;
            std::cout << "status" << s<<std::endl;
            std::cout << "side" << str<<std::endl;
            std::cout << "trade_id" <<trade_id <<std::endl;
        }
    };


};
