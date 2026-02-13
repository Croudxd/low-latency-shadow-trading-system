#pragma once
#include <cstdint>
#include <iostream>
#include <string>

namespace Rep 
{
    enum class Status 
    {
        NEW,
        FILLED,
        PARTIALLY_FILLED,
        CANCELLED,
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
        uint64_t last_quantity; // Shares just traded.
        int64_t last_price; // price traded at.
        uint64_t leaves_quantity; // Amount on book.
        Side side; //bull or sold.
        Rejection_code reject_code; // Rejection code.
        uint64_t trade_id; // Trade id (index of the vector?)
        uint64_t timestamp; //Time

        void print() const
        {
            std::string s = (status == Status::CANCELLED) ? "cancelled" : "fuck knows";
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
}
