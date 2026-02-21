#pragma once
#include <cstdint>

namespace backtester 
{

    struct Dashboard_state 
    {
        volatile double cash;
        volatile double locked_cash;
        volatile double position;
        volatile double total_fees;
        volatile double pnl;
        
        volatile uint64_t active_order_count;
        volatile uint64_t total_trades;
        volatile uint64_t last_update_ts;

        volatile double last_trade_price;
        volatile uint64_t last_trade_id;
    };
}
