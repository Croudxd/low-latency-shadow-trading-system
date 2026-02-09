#pragma once
#include <iostream>
class Candle
{
    public:
        Candle() = default;
        Candle(long _open, long _high, long _low, long _close, long _volume) : open(_open), high(_high), low(_low), close(_close), volume(_volume)
        {
        }
        void print()
        {
            std::cout << "[CANDLE]:" << std::endl
                << "OPEN: " << open<< std::endl
                << "HIGH: " << high<< std::endl
                << "LOW: " << low << std::endl
                << "CLOSE: " << close<< std::endl
                << "VOLUME: " << volume << std::endl;
        }

    private:
        long open;
        long high;
        long low;
        long close;
        long volume;

};
