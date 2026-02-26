#pragma once
#include <cstdint>
#include <iostream>
namespace common
{
    struct Candle
    {
        Candle() = default;
        Candle(int64_t _open, int64_t _high, int64_t _low, int64_t _close, int64_t _volume, uint64_t _time) : open(_open), high(_high), low(_low), close(_close), volume(_volume), time(_time)
        {
        }

        Candle(int64_t _open, int64_t _high, int64_t _low, int64_t _close, int64_t _volume) : open(_open), high(_high), low(_low), close(_close), volume(_volume)
        {
        }
        int64_t get_open() const { return open; }

        int64_t open;
        int64_t high;
        int64_t low;
        int64_t close;
        int64_t volume;
        uint64_t time;

        void print()
        {
            std::cout << "open" << open <<std::endl;
            std::cout << "high" << high <<std::endl;
            std::cout << "low" << low <<std::endl;
            std::cout << "close" << close <<std::endl;
            std::cout << "volume" << volume <<std::endl;
            std::cout << "time" << time <<std::endl;
        }

    };
}
