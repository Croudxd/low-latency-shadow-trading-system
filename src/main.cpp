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
#include "candle.hpp"
#include "order_book.hpp"

#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <limits>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>

/** multiple by 1 million because rust feeder does this to avoid floats. So just normalizing*/
static constexpr int BUCKET_SIZE = 1 * 100000; 
static constexpr int BUFFER_CAPACITY = 16384; 

struct Data 
{
    uint64_t   id ; 
    uint64_t   size ; 
    int32_t    price ; 
    int8_t     side ;  
    int8_t     action ;
    int8_t     status ;
    uint8_t    pad1[1];
};

struct Shared_memory_layout 
{
    volatile uint64_t write_idx; 
    uint8_t           pad1[56];
    volatile uint64_t read_idx; 
    uint8_t           pad2[56];
    Data              buffer[BUFFER_CAPACITY];
};

struct Candle_shared_memory_layout 
{
    volatile uint64_t write_idx; 
    uint8_t           pad1[56];
    volatile uint64_t read_idx; 
    uint8_t           pad2[56];
    Candle            buffer[BUFFER_CAPACITY];
};


void send_candle(Candle_shared_memory_layout* candle_shm, Candle candle)
{
    uint64_t local_write_idx = candle_shm->write_idx;
    uint64_t cached_read_idx = candle_shm->read_idx; 
        if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
        {
            cached_read_idx = candle_shm->read_idx;
            
        }
        if (local_write_idx - cached_read_idx >= BUFFER_CAPACITY) 
        {
            return;
        }
 
        candle_shm->buffer[local_write_idx % BUFFER_CAPACITY] = candle;
        std::atomic_thread_fence(std::memory_order_release);
        candle_shm->write_idx = local_write_idx + 1;
}

int main() 
{
    Order_book book = Order_book{};
    int fd = open("/dev/shm/hft_ring", O_RDWR);
    while (fd == -1) 
    {
        std::cout << "Waiting for Rust feeder...\n";
        sleep(1);
        fd = open("/dev/shm/hft_ring", O_RDWR);
    }
    void* ptr = mmap(NULL, sizeof(Shared_memory_layout), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) 
    {
        perror("mmap failed"); 
        return 1;
    }
    Shared_memory_layout* shm = static_cast<Shared_memory_layout*>(ptr);
    uint64_t local_read_idx = shm->write_idx;
    shm->read_idx = local_read_idx;
    std::cout << "Connected! Watching memory...\n";


    int candle_fd = open("/dev/shm/hft_candle", O_RDWR | O_CREAT, 0666); 
    ftruncate(candle_fd, sizeof(Candle_shared_memory_layout)); 
    void* candle_ptr = mmap(NULL, sizeof(Candle_shared_memory_layout), PROT_READ | PROT_WRITE, MAP_SHARED, candle_fd, 0);
    if (candle_ptr == MAP_FAILED) { perror("mmap failed"); return 1; }
    Candle_shared_memory_layout* candle_shm = static_cast<Candle_shared_memory_layout*>(candle_ptr);

    size_t count = 0;
    std::cout << "Producer starting..." << std::endl;



    long local_bucket_size= 0;
    long open = 0;
    long high = 0;
    long low = std::numeric_limits<long>::max();
    long close = 0;

    while (true)
    {
        uint64_t current_write_idx = shm->write_idx;
        if (local_read_idx < current_write_idx) 
        {
            int slot = local_read_idx % BUFFER_CAPACITY;
            Data raw = shm->buffer[slot];
            auto side = (raw.side == 0) ? Order_type::buy : Order_type::sell;
            Order ord = {side, raw.price, raw.size, raw.id};
            /** status = 1 is a trade not an order so we dont add to the book.*/
            if (raw.status == 1)
            {
                std::cout << "." << std::flush; 
                if (open == 0)
                {
                    open = raw.price;
                }
                if (raw.price > high)
                {
                    high = raw.price;
                } 
                if (raw.price < low)
                {
                    low = raw.price;
                } 
                local_bucket_size += raw.size;
                if (local_bucket_size >= BUCKET_SIZE)
                {
                    close = raw.price; 
                    Candle candle = Candle{open, high, low, close, local_bucket_size};
                    candle.print();
                    send_candle(candle_shm, candle);
                    local_bucket_size = 0;
                    open = 0;
                    high = 0;
                    low = std::numeric_limits<long>::max();
                }
            }
            else if (raw.action == 2)
            {
                book.cancel_order(raw.id);
            }
            else 
            {
                book.add_order(ord, Flags::NONMATCH);
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


