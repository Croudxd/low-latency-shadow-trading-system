#pragma once
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include "report.hpp"

namespace backtester
{
    struct Portfolio
    {
        double starting_cash;
        double cash;
        double locked_cash;
        double position;
        double total_fees;
        double fee_rate;

        std::unordered_map<uint64_t, double> order_locks;

        Portfolio(double start, double fee) 
            : starting_cash(start)
            , cash(start)
            , locked_cash(0.0)
            , position(0.0)
            , total_fees(0.0)
            , fee_rate(fee)
        {}

        double get_equity(double current_price) const 
        {
            return cash + locked_cash + (position * (current_price / 100.0));
        }

        double get_cash() const 
        {
            return cash; 
        }

        double get_position() const
        {
            return position;
        }

        void update(const Report& rep, double limit_price)
        {
            double real_limit_price = limit_price / 100.0;
            double real_last_price  = (double)rep.last_price / 100.0;
            
            double real_last_qty    = (double)rep.last_quantity    / 1000000.0;
            double real_leaves_qty  = (double)rep.leaves_quantity / 1000000.0;


            if (rep.status == Status::NEW)
            {
                if (rep.side == Side::BUY) 
                {
                    double amount_to_lock = real_limit_price * real_leaves_qty;
                    cash -= amount_to_lock;
                    locked_cash += amount_to_lock;
                    order_locks[rep.order_id] = amount_to_lock;
                }
            }
            else if (rep.status == Status::FILLED || rep.status == Status::PARTIALLY_FILLED)
            {
                double trade_value = real_last_price * real_last_qty;
                
                if (rep.side == Side::BUY) 
                {
                    position += real_last_qty;

                    if (order_locks.count(rep.order_id)) 
                    {
                        double lock_portion = real_limit_price * real_last_qty;
                        
                        locked_cash -= lock_portion;
                        cash += (lock_portion - trade_value); 
                        order_locks[rep.order_id] -= lock_portion;
                    }
                    else 
                    {
                        cash -= trade_value;
                    }
                }
                else 
                { 
                    position -= real_last_qty;
                    cash     += trade_value;
                }
                
                double local_fee = trade_value * fee_rate;
                total_fees += local_fee;
                cash -= local_fee;
            }
        }
    };
}
