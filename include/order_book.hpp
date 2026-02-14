#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>

#include "order.hpp"
#include "report.hpp"
#include "trade.hpp"
#include "utils.hpp"

#ifdef UNIT_TEST
class strategy_order_handles_strategy_data_Test;
class add_order_check_book_Test;
class rust_feeder_handles_rust_data_Test;
class cancel_order_check_book_Test;
class check_match_check_book_Test;
class execute_match_check_book_Test;
#endif

enum class Flags
{
    MATCH,
    NONMATCH,
};

class Order_book
{
#ifdef UNIT_TEST
    friend class ::strategy_order_handles_strategy_data_Test;
    friend class ::add_order_check_book_Test;
    friend class ::rust_feeder_handles_rust_data_Test;
    friend class ::cancel_order_check_book_Test;
    friend class ::check_match_check_book_Test;
    friend class ::execute_match_check_book_Test;
#endif

    friend class Order;

    struct Order_entry
    {
        Order* location;
        bool   is_buy;
    };

    std::map<int64_t, std::deque<Order>>  bids;
    std::map<int64_t, std::deque<Order>>  asks;
    std::map<int64_t, std::deque<Order>>* lookup[2];

    std::vector<Trade>                      trade_history;
    std::unordered_map<size_t, Order_entry> order_lookup;
    uint64_t                                trade_id = 0;

public:
    Order_book()
    {
        lookup[0] = &bids;
        lookup[1] = &asks;
        order_lookup.reserve(100000);
    }

    template <typename Func> void add_order(Order order, Flags flag, Func on_report)
    {
        if (flag == Flags::MATCH)
        {
            while (order.size > 0)
            {
                if (!check_match(order))
                {
                    break;
                }

                Rep::Report repo = execute(order, on_report);

                if (repo.status == Rep::Status::FILLED || repo.status == Rep::Status::PARTIALLY_FILLED)
                {
                    on_report(repo);
                }

                if (repo.last_quantity == 0)
                    break;
            }
        }

        if (order.size > 0)
        {
            uint64_t saved_id = order.ID;
            bool     is_buy   = (order.type == Order_type::buy);

            int64_t storage_price = (is_buy) ? (order.price * -1) : order.price;
            order.price           = storage_price;

            Order* stored_ptr = nullptr;

            if (is_buy)
            {
                bids[storage_price].push_back(order);
                stored_ptr = &bids[storage_price].back();
            }
            else
            {
                asks[storage_price].push_back(order);
                stored_ptr = &asks[storage_price].back();
            }

            order_lookup[saved_id] = Order_entry { stored_ptr, is_buy };

            Rep::Report rep(saved_id, Rep::Status::NEW, 0, 0, order.size, (is_buy ? Rep::Side::BUY : Rep::Side::SELL),
                Rep::Rejection_code::NOERROR, 0, 0);
            on_report(rep);
        }
    }

    bool check_match(const Order& order)
    {
        bool  is_buy      = (order.type == Order_type::buy);
        auto* target_book = (is_buy) ? &asks : &bids;

        if (target_book->empty())
            return false;

        int64_t best_price = target_book->begin()->first;

        if (is_buy)
        {
            return order.price >= best_price;
        }
        else
        {
            return order.price <= std::abs(best_price);
        }
    }

    template <typename Func> void cancel_order(size_t ID, Func on_report)
    {
        auto lookup_it = order_lookup.find(ID);
        if (lookup_it == order_lookup.end())
            return;

        Order* order_ptr = lookup_it->second.location;
        bool   is_buy    = lookup_it->second.is_buy;

        if (order_ptr)
        {
            order_ptr->active = false;
        }

        Rep::Report report(ID, Rep::Status::CANCELLED, 0, 0, 0, (is_buy ? Rep::Side::BUY : Rep::Side::SELL),
            Rep::Rejection_code::NOERROR, 0, cstime::get_timestamp());
        on_report(report);

        order_lookup.erase(lookup_it);
    }

    std::vector<Trade> get_trade_history()
    {
        return this->trade_history;
    }

private:
    template <typename Func> 
    Rep::Report execute(Order& taker, Func on_report)
    {
        bool  is_buy_taker = (taker.type == Order_type::buy);
        auto* book_side    = (is_buy_taker) ? &asks : &bids;

        uint64_t total_filled = 0;
        int64_t  last_exec_price = 0;
        while (taker.size > 0 && !book_side->empty())
        {
            auto               best_price_it = book_side->begin();
            std::deque<Order>& level         = best_price_it->second;

            if (!level.empty() && !level.front().active)
            {
                level.pop_front();
                if (level.empty())
                {
                    book_side->erase(best_price_it);
                }
                continue;
            }

            if (level.empty())
            {
                book_side->erase(best_price_it);
                continue;
            }

            Order&  maker           = level.front();
            int64_t maker_price_abs = std::abs(maker.price);

            if (is_buy_taker)
            {
                if (taker.price < maker_price_abs)
                    break;
            }
            else
            {
                if (taker.price > maker_price_abs)
                    break;
            }

            uint64_t trade_qty   = std::min(taker.size, maker.size);
            int64_t  trade_price = maker_price_abs;

            last_exec_price = trade_price;

            taker.size -= trade_qty;
            maker.size -= trade_qty;
            total_filled += trade_qty;
            trade_id++;

            Rep::Status maker_status = (maker.size == 0) ? Rep::Status::FILLED : Rep::Status::PARTIALLY_FILLED;
            Rep::Report maker_rep(maker.ID, maker_status, trade_qty, trade_price, maker.size,
                (is_buy_taker ? Rep::Side::SELL : Rep::Side::BUY), Rep::Rejection_code::NOERROR, trade_id,
                cstime::get_timestamp());
            on_report(maker_rep);

            if (maker.size == 0)
            {
                order_lookup.erase(maker.ID);
                level.pop_front();
                if (level.empty())
                {
                    book_side->erase(best_price_it);
                }
            }
        }

        Rep::Status taker_status = (taker.size == 0) ? Rep::Status::FILLED : Rep::Status::PARTIALLY_FILLED;

        return Rep::Report(taker.ID, taker_status, total_filled, last_exec_price, taker.size,
            (is_buy_taker ? Rep::Side::BUY : Rep::Side::SELL), Rep::Rejection_code::NOERROR, trade_id,
            cstime::get_timestamp());
    }
};
