#include <iostream>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>

#include "../include/dashboard.hpp"

using namespace backtester;

int main() {
    int fd = -1;
    while (fd == -1) {
        fd = open("/dev/shm/hft_dashboard", O_RDWR);
        if (fd == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    Dashboard_state* state = (Dashboard_state*)mmap(NULL, sizeof(Dashboard_state), 
                                                PROT_READ, MAP_SHARED, fd, 0);

    std::cout << "\033[2J"; 
    while (true) {
        std::cout << "\033[H"; 

        std::cout << "=== MONITOR ===\n";
        std::cout << "---------------------------------------------\n";
        
        std::cout << " CASH:       \033[1;32m$" << std::fixed << std::setprecision(2) 
                  << state->cash << "\033[0m" << std::endl;
                  
        std::cout << " LOCKED:     $" << state->locked_cash << std::endl;
        
        std::cout << " POSITION:   \033[1;36m" << state->position << "\033[0m units" << std::endl;
        
        std::cout << " FEES PAID:  \033[1;31m$" << state->total_fees << "\033[0m" << std::endl;
        std::cout << "---------------------------------------------\n";
        
        std::cout << " ACTIVE ORDS: " << state->active_order_count << std::endl;
        std::cout << " TTL TRADES:  " << state->total_trades << std::endl;
        std::cout << " LAST TRADE:  #" << state->last_trade_id << " @ $" << state->last_trade_price << std::endl;
        
        std::cout << "---------------------------------------------\n";
        std::cout << " ENGINE TS:   " << state->last_update_ts << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
