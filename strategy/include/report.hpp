#pragma once
#include <cstdint>
#include <iostream>

namespace backtester
{
    enum class Status 
    {
        NEW,
        FILLED,
        PARTIALLY_FILLED,
        CANCELED,
        REJECTED,
    };

    enum class Side
    {
        BUY,
        SELL,
    };

    enum class Rejection_code
    {
        NOERROR,
        NO_FUNDS,
        PRICE_OUT_OF_BOUNDS,
        SYSTEM_ERROR,
    };

    struct Report 
    {
        uint64_t order_id;
        Status status;
        uint64_t last_quantity;
        uint64_t last_price;
        uint64_t leaves_quantity;
        Side side;
        Rejection_code reject_code;
        uint64_t trade_id;
        uint64_t timestamp;

        void print() const
        {
            std::string s = (status == Status::CANCELED) ? "cancelled" : "fuck knows";
            std::string str = (side == Side::BUY) ? "buy" : "sell";

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

    struct Active_orders 
    {
        uint64_t order_id;
        uint64_t leaves_quantity;
        uint64_t price;
        Side side;
        uint64_t timestamp;

        void print() const
        {

            std::cout << "order_id" << order_id <<std::endl;
            std::cout << "order_id" << order_id <<std::endl;
            std::cout << "leaves_quantity" << leaves_quantity<<std::endl;
            std::cout << "price" << price <<std::endl;
        }
    };

};
