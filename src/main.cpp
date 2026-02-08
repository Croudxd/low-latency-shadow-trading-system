/** 
 * Okay we have two parts to an order book.
 * The bids which is sorted from highest to lowest (highest at the top)
 * and the asks which is sorted lowest to highest.
 *
 * The gap between bid and asks is called the spread.
 *
 * At each price point we might have multiple orders, so we store them in time order (FIFO)
 * When we get an incoming bid. we try and match. If it matches we call execute.
 * Else we add the bid into the queue.
 *
 * A user might want to cancel an order, so we need to implement a way to find and cancel it.
 *
 * we might have a market order, basically buy x amount of shares without caring for price.
 * So we pick the top 
 *
 * an order needs. An ID, price, size and a side (buying or selling)
 * */

#include "order.hpp"
#include "order_book.hpp"

#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>

struct Data 
{
    uint64_t id; 
    uint64_t size ; 
    int32_t  price; 
    int8_t side ;  
    int8_t action ;
    uint8_t pad1[2];
};

struct SharedMemoryLayout 
{
    volatile uint64_t write_idx; 
    uint8_t pad1[56];
    volatile uint64_t read_idx; 
    uint8_t pad2[56];
    Data buffer[16384];
};

int main() 
{
    Order_book book = Order_book{};
    int fd = open("/dev/shm/hft_ring", O_RDWR);
    while (fd == -1) {
        std::cout << "Waiting for Rust (Run the feeder!)...\n";
        sleep(1);
        fd = open("/dev/shm/hft_ring", O_RDWR);
    }

    void* ptr = mmap(NULL, sizeof(SharedMemoryLayout), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (ptr == MAP_FAILED) 
    {
        perror("mmap failed"); 
        return 1;
    }
    SharedMemoryLayout* shm = static_cast<SharedMemoryLayout*>(ptr);

    uint64_t local_read_idx = shm->write_idx;
    shm->read_idx = local_read_idx;
    std::cout << "Connected! Watching memory...\n";

    while (true)
    {
        uint64_t current_write_idx = shm->write_idx;

        if (local_read_idx < current_write_idx) 
        {
            int slot = local_read_idx % 16384;
            Data raw = shm->buffer[slot];

            auto side = (raw.side == 0) ? Order_type::buy : Order_type::sell;
            Order ord = {side, raw.price, raw.size, raw.id};
            if (raw.action == 1)
            {
                std::cout << "[DEL] " << raw.id << std::endl;
                book.cancel_order(raw.id);
            }
            else 
            {
                std::cout << "[ADD] " << raw.id << " @ " << raw.price << std::endl;
                book.add_order(ord);
            }
            
            for (auto& trade : book.get_trade_history())
            {
                std::string side = (trade.type == Order_type::sell) ? "sell" : "buy";
                std::cout << "Trade: " << side << " executed @ " << trade.price << " Size: " << trade.size << std::endl;
            }
            local_read_idx++;
            
            shm->read_idx = local_read_idx; 
            
        }
        else 
        {
            std::this_thread::yield();
        }
    }
}

