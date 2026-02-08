#pragma once
#include <cstddef>
enum Order_type
{
    buy,
    sell,
    null
};

class Order 
{
    friend class Order_book;
    public:
        Order () = default;

        Order(Order_type _type, long _price, size_t _size, size_t _ID) 
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

    private:
        Order_type type;
        long price;
        size_t size;
        size_t ID;

};
