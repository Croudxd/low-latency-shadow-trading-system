#pragma once
#include <cstddef>
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>

#include "order.hpp"
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

        void add_order (Order order, Flags flag )
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
                            execute(order);
                        }
                    }
                }
            }
            if (order.size > 0)
            {
                auto [ it, inserted ] = lookup[order.type]->try_emplace(order.price);
                it->second.push_back(std::move(order));
                auto list_it = std::prev(it->second.end());

                order_lookup[list_it->ID] = Order_entry{ list_it, (order.type == Order_type::buy)};
                return;
            }
        }

        bool check_match(Order order)
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

        void cancel_order (size_t ID)
        {
            auto lookup_it = order_lookup.find(ID);
            if ( lookup_it == order_lookup.end())
            {
                return;
            }

            auto& it = lookup_it->second;

            long price = it.location->price;
            bool map = (it.is_buy) ? 0 : 1;

            auto price_it = lookup[map]->find(price);
            if ( price_it  != lookup[map]->end() )
            {
                price_it->second.erase(it.location);
                if(price_it->second.empty())
                {
                    lookup[map]->erase(price_it);
                }
            }

            order_lookup.erase(lookup_it);
        }

        std::vector<Trade> get_trade_history() { return this->trade_history; }

    private:
        void execute (Order& order) 
        {
            int type = (order.type == 1) ? 0 : 1;
            Order& book_order = lookup[type]->begin()->second.front();

            long trade_size = (order.size < book_order.size) ? order.size : book_order.size;
            long trade_price = book_order.price;

            order.size -= trade_size;
            book_order.size -= trade_size;

            if (order.type == Order_type::sell)
            {
                trade_price = book_order.price *-1;
            }

            trade_history.emplace_back(cstime::get_timestamp(), trade_size, trade_price, order.type);

            if (book_order.size == 0)
            {
                order_lookup.erase(book_order.ID);
                lookup[type]->begin()->second.pop_front();
                if (lookup[type]->begin()->second.empty())
                {
                    lookup[type]->erase(lookup[type]->begin());
                }
            }

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

};
