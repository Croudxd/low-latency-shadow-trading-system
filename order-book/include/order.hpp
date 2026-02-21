#pragma once
#include <cstddef>
#include <cstdint>
#include <iostream>

#ifdef UNIT_TEST
class strategy_order_handles_strategy_data_Test;
class rust_feeder_handles_rust_data_Test;
class add_order_check_book_Test;
class cancel_order_check_book_Test;
class check_match_check_book_Test;
class execute_match_check_book_Test;
#endif

enum Order_type
{
    buy,
    sell,
    null
};

class Order 
{
    
    #ifdef UNIT_TEST
    friend class ::strategy_order_handles_strategy_data_Test;
    friend class ::rust_feeder_handles_rust_data_Test;
    friend class ::add_order_check_book_Test;
    friend class ::cancel_order_check_book_Test;
    friend class ::check_match_check_book_Test;
    friend class ::execute_match_check_book_Test;
    #endif

    friend class Order_book;
    public:
        bool active = true;
        Order () = default;

        Order(Order_type _type, int64_t _price, uint64_t _size, uint64_t _ID) 
            : type(_type), price(_price), size(_size), ID(_ID)
        {

        }
        ~Order()
        {

        }

        Order(Order&& other)
        {
            this->type = other.type;
            this->price = other.price;
            this->size = other.size;
            this->ID = other.ID;

            other.type = Order_type::null;
            other.price = 0;
            other.size = 0;
            other.ID = 0;
        }

        Order(const Order& other)
        {
            this->type = other.type;
            this->price = other.price;
            this->size = other.size;
            this->ID = other.ID;
        }

        void print()
        {
            std::cout << "ID: " << ID << std::endl;
            std::cout << "price: " << price << std::endl;
            std::cout << "size: " << size << std::endl;
        }

    private:
        Order_type type;
        int64_t price;
        uint64_t size;
        uint64_t ID;

};
