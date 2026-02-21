#pragma once
#include <iostream>
class Candle
{
    public:
        Candle() = default;
        Candle(long _open, long _high, long _low, long _close, long _volume) : open(_open), high(_high), low(_low), close(_close), volume(_volume)
        {
        }

        long get_open() const
        {
            return open;
        }

        void print()
        {
            std::cout << "OPEN: " << open << std::endl;
            std::cout << "high: " << high << std::endl;
            std::cout << "low: " << low << std::endl;
            std::cout << "close: " << close << std::endl;
            std::cout << "volume: " << volume << std::endl;
        }
    private:
        long open;
        long high;
        long low;
        long close;
        long volume;

};
