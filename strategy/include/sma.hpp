#pragma once
#include "indicator.hpp"
#include <cstddef>

namespace backtester
{
    class SMA : public Indicator 
    {
    public:
        SMA(size_t size) : period(size) {}

        double calculate() 
        {
            size_t count = ring_buffer->size();

            if (count < period)
            {
                return 0.0;
            }
            else if (count == period) 
            {
                sum = 0.0;
                for (size_t i = 0; i < period; i++)
                {
                    sum += ring_buffer->get(i).get_open();
                }
                initialized = true;
            }
            if (!initialized || (count > last_count_size + 1))
            {
                sum = 0.0;
                for (size_t i = 0; i < period; i++)
                {
                    sum += ring_buffer->get(i).get_open();
                }
                initialized = true;
            }
            else if (count > last_count_size)
            {
                double incoming = ring_buffer->get(0).get_open();      
                double outgoing = ring_buffer->get(period).get_open();
                
                sum = sum + incoming - outgoing;
            }

            last_count_size = count;
            
            return sum / period;
        }

    private:
        double sum = 0;
        size_t period = 0;
        size_t last_count_size = 0;
        bool initialized = false; 
    };
};
