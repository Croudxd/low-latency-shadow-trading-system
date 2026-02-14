#include <gtest/gtest.h>

#include "order.hpp"
#include "order_book.hpp"

TEST(add_order, check_book)
{
    auto sender = [&](const Rep::Report& rep) 
    {
    };

    Order_book book = Order_book();
    {
        Order ord = Order(Order_type::sell, 123, 12, 123);
        book.add_order(ord, Flags::MATCH, sender);
        auto lookup_it = book.order_lookup.find(ord.ID);
        ASSERT_EQ(lookup_it->second.location->ID, ord.ID);
        ASSERT_EQ(lookup_it->second.location->price, 123);
        ASSERT_EQ(lookup_it->second.location->size, ord.size);
        ASSERT_EQ(lookup_it->second.location->type, ord.type);
    }
    {
        Order ord = Order(Order_type::buy, 123, 12, 124);
        book.add_order(ord, Flags::MATCH, sender);
        auto lookup_it = book.order_lookup.find(ord.ID);
        ASSERT_EQ(lookup_it, book.order_lookup.end());
    }
    {
        Order ord = Order(Order_type::buy, 123, 12, 125);
        book.add_order(ord, Flags::NONMATCH, sender);
        auto lookup_it = book.order_lookup.find(ord.ID);
        ASSERT_EQ(lookup_it->second.location->ID, ord.ID);
        ASSERT_EQ(lookup_it->second.location->price, -123);
        ASSERT_EQ(lookup_it->second.location->size, ord.size);
        ASSERT_EQ(lookup_it->second.location->type, ord.type);

        Order ord1 = Order(Order_type::buy, 123, 12, 126);
        book.add_order(ord1, Flags::NONMATCH, sender);
        auto lookup_it1 = book.order_lookup.find(ord1.ID);
        ASSERT_EQ(lookup_it1->second.location->ID, ord1.ID);
        ASSERT_EQ(lookup_it1->second.location->price, -123);
        ASSERT_EQ(lookup_it1->second.location->size, ord1.size);
        ASSERT_EQ(lookup_it1->second.location->type, ord1.type);
    }
}

TEST(cancel_order, check_book)
{
    auto sender = [&](const Rep::Report& rep) 
    {
    };

    Order_book book = Order_book();
    {
        Order ord = Order(Order_type::sell, 123, 12, 123);
        book.add_order(ord, Flags::MATCH, sender);

        book.cancel_order(123, sender);
        auto lookup_it1 = book.order_lookup.find(ord.ID);
        ASSERT_EQ(lookup_it1, book.order_lookup.end());
    }
    {
        Order ord = Order(Order_type::buy, 123, 12, 123);
        book.add_order(ord, Flags::MATCH, sender);

        book.cancel_order(123, sender);
        auto lookup_it1 = book.order_lookup.find(ord.ID);
        ASSERT_EQ(lookup_it1, book.order_lookup.end());
    }
}

TEST(check_match, check_book)
{
    auto sender = [&](const Rep::Report& rep) 
    {
    };

    Order_book book = Order_book();
    {
        Order ord = Order(Order_type::buy, 123, 12, 123);
        book.add_order(ord, Flags::MATCH, sender);
        Order ord1 = Order(Order_type::sell, 123, 12, 123);

        ASSERT_EQ(book.check_match(ord1), true);
    }
    {
        Order ord = Order(Order_type::buy, 123, 12, 123);
        book.add_order(ord, Flags::MATCH, sender);
        Order ord1 = Order(Order_type::sell, 126, 12, 123);

        ASSERT_EQ(book.check_match(ord1), false);
    }
}

TEST(execute_match, check_book)
{
    bool maker_callback_called = false;
    uint64_t maker_fill_qty = 0;
    Rep::Status maker_status = Rep::Status::NEW;

    auto sender = [&](const Rep::Report& rep) 
    {
        maker_callback_called = true;
        maker_fill_qty = rep.last_quantity;
        maker_status = rep.status;
        
    };

    Order_book book;

    {
        Order ord = Order(Order_type::buy, 123, 12, 123);
        
        book.add_order(ord, Flags::NONMATCH, sender); 

        Order ord1 = Order(Order_type::sell, 123, 12, 124);
        Rep::Report taker_rep = book.execute(ord1, sender);

        ASSERT_EQ(taker_rep.order_id, ord1.ID);
        ASSERT_EQ(taker_rep.status, Rep::Status::FILLED); 
        ASSERT_EQ(taker_rep.last_quantity, 12);          
        ASSERT_EQ(taker_rep.last_price, 123);           

        ASSERT_TRUE(maker_callback_called);
        ASSERT_EQ(maker_status, Rep::Status::FILLED);  
        ASSERT_EQ(maker_fill_qty, 12);

        auto it = book.order_lookup.find(ord.ID);
        ASSERT_EQ(it, book.order_lookup.end());
    }

    {
        Order ord = Order(Order_type::buy, 123, 12, 125);
        book.add_order(ord, Flags::NONMATCH, sender);

        Order ord1 = Order(Order_type::sell, 126, 12, 126);

        ASSERT_EQ(book.check_match(ord1), false);
    }
}
