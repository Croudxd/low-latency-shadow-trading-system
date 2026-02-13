#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>

#include "order.hpp"
#include "report.hpp"
#include "trade.hpp"
#include "utils.hpp"

enum class Flags
{
    MATCH,
    NONMATCH,
};

class Order_book
{
    friend class Order;
    public:
        Order_book ()
        {
            lookup[0] = &bids;
            lookup[1] = &asks;
        }
        template <typename Func>
        void add_order (Order order, Flags flag, Func on_report)
        {
            (order.type == 0) ? order.price *= -1 : order.price;
            if (flag == Flags::MATCH)
            {
                bool matched = check_match(order);

                if (matched == true)
                {
                    if (!lookup[(order.type == 1) ? 0 : 1]->empty() && !lookup[(order.type == 1) ? 0 : 1]->begin()->second.empty())
                    {

                        while ( order.size > 0)
                        {
                            if (lookup[(order.type == 1) ? 0 : 1]->empty())
                            {
                                break;
                            }
                            if (!check_match(order))
                            {
                                break;
                            }
                            Rep::Report repo = execute(order, on_report);
                            on_report(repo);
                        }
                    }
                }
            }
            if (order.size > 0)
            {
                uint64_t saved_id    = order.ID;
                long     saved_price = order.price;
                long     saved_size  = order.size;
                int      saved_type  = order.type;
                bool     is_buy      = (saved_type == 0); 

                auto [ it, inserted ] = lookup[saved_type]->try_emplace(saved_price);
                
                it->second.push_back(std::move(order));
                auto list_it = std::prev(it->second.end());

                order_lookup[saved_id] = Order_entry{ list_it, is_buy };

                Rep::Side side = (saved_type == 1) ? Rep::Side::SELL : Rep::Side::BUY;

                long report_price = (saved_price < 0) ? saved_price * -1 : saved_price;

                Rep::Report repo = Rep::Report(
                    saved_id, 
                    Rep::Status::NEW, 
                    0, 
                    report_price / 100.0, 
                    saved_size / 1000000.0, 
                    side, 
                    Rep::Rejection_code::NOERROR, 
                    0, 
                    cstime::get_timestamp()
                );
                
                if (flag == Flags::MATCH) on_report(repo);
                return;
            }
        }

        bool check_match(const Order& order)
        {
           int type = (order.type == 1) ? 0 : 1;
            if (lookup[type]->empty()) return false;
            bool match = false;
            auto it = lookup[type]->begin();

            if (order.type == Order_type::sell)
            {
                match = order.price <= it->first*-1;
            }
            if (order.type == Order_type::buy)
            {
                match = order.price*-1 >= it->first ;
            }
            return match;
        }

        template <typename Func>
        void cancel_order (size_t ID, Func on_report)
        {
            auto lookup_it = order_lookup.find(ID);
            if ( lookup_it == order_lookup.end())
            {
                return;
            }

            auto& it = lookup_it->second;

            long price = it.location->price;
            bool map = (it.is_buy) ? 0 : 1;

            Rep::Side side = (it.is_buy) ? Rep::Side::BUY : Rep::Side::SELL;


            auto price_it = lookup[map]->find(price);
            if ( price_it  != lookup[map]->end() )
            {
                price_it->second.erase(it.location);
                if(price_it->second.empty())
                {
                    lookup[map]->erase(price_it);
                }
            }

            Rep::Report report = Rep::Report(ID, Rep::Status::CANCELLED, 0, 0, 0, side, Rep::Rejection_code::NOERROR, 0 /**trade id*/, cstime::get_timestamp());
            on_report(report);
            order_lookup.erase(lookup_it);
        }

        std::vector<Trade> get_trade_history() { return this->trade_history; }

    private:
        template<typename Func>
        Rep::Report execute (Order& order, Func on_report) 
        {
            int type = (order.type == 1) ? 0 : 1;
            Order& book_order = lookup[type]->begin()->second.front();

            if (book_order.size <= 0 || std::abs(book_order.price) == 0) 
            {
                lookup[type]->begin()->second.pop_front();
                if (lookup[type]->begin()->second.empty()) {
                    lookup[type]->erase(lookup[type]->begin());
                }
                return execute(order, on_report); 
            }
            uint64_t trade_size = (order.size < book_order.size) ? order.size : book_order.size;
            int64_t trade_price = std::abs(book_order.price);

            order.size -= trade_size;
            book_order.size -= trade_size;


            trade_history.emplace_back(cstime::get_timestamp(), trade_size, trade_price, order.type);
            trade_id++;

            Rep::Status maker_status = (book_order.size > 0) ? Rep::Status::PARTIALLY_FILLED : Rep::Status::FILLED;
            Rep::Side   maker_side   = (book_order.type == Order_type::sell) ? Rep::Side::SELL : Rep::Side::BUY;

            Rep::Report maker_rep(
                book_order.ID,
                maker_status,
                trade_size ,
                trade_price ,
                book_order.size,
                maker_side,
                Rep::Rejection_code::NOERROR,
                trade_id,
                cstime::get_timestamp()
            );
            // maker_rep.print();
            on_report(maker_rep); 

            if (book_order.size == 0)
            {
                order_lookup.erase(book_order.ID);
                lookup[type]->begin()->second.pop_front(); 
                
                if (lookup[type]->begin()->second.empty())
                {
                    lookup[type]->erase(lookup[type]->begin());
                }
            }

    Rep::Status taker_status = (order.size > 0) ? Rep::Status::PARTIALLY_FILLED : Rep::Status::FILLED;
    Rep::Side   taker_side   = (order.type == Order_type::sell) ? Rep::Side::SELL : Rep::Side::BUY;

    return Rep::Report(
        order.ID, 
        taker_status, 
        trade_size, 
        trade_price, 
        order.size, 
        taker_side, 
        Rep::Rejection_code::NOERROR, 
        trade_id, 
        cstime::get_timestamp()
    );
}

        struct Order_entry
        {
            std::list<Order>::iterator location;
            bool is_buy;
        };

        std::map<long, std::list<Order>> bids;
        std::map<long, std::list<Order>> asks;
        std::map<long, std::list<Order>>* lookup[2];
        std::vector<Trade> trade_history;
        std::unordered_map<size_t, Order_entry> order_lookup;
        uint64_t trade_id = 0;
};
