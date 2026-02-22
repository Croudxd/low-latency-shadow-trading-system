#pragma once
#include <iostream>

struct Candle
{
    Candle() = default;
    Candle(long _open, long _high, long _low, long _close, long _volume) : open(_open), high(_high), low(_low), close(_close), volume(_volume)
    {
    }

    long open;
    long high;
    long low;
    long close;
    long volume;

};
