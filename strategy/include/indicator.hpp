#pragma once

#include "ring_buffer.hpp"

namespace backtester
{
    class Indicator 
    {
        public:
            static void set_ring_buffer(backtester::Ring_buffer* _ring_buffer)
            {
                ring_buffer = _ring_buffer;
            }
        protected:
            inline static backtester::Ring_buffer* ring_buffer = nullptr;

    };

};
