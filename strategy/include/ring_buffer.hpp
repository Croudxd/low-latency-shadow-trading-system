#pragma once
#include <candle.hpp>

namespace backtester
{
    class Ring_buffer
    {
        public:
            Ring_buffer () = default;
            void add(const Candle& item) 
            {
                history[head] = item;
                head = (head + 1) % max_size; 
                if (count < max_size) count++;
            }

            const Candle& get(size_t index) const 
            {
                if (index >= count) throw std::out_of_range("History too short");
                    size_t raw_idx = (head - 1 - index + max_size) % max_size;
                    return history[raw_idx];
            }

            size_t size() const
            {
                return count;
            }
        private:
            size_t head = 0;
            size_t count = 0;
            static constexpr size_t max_size = 16384; 
            Candle history[max_size];
    };
};
