#pragma once
#include <cstdint>
namespace common
{
    struct Candle
    {
        Candle() = default;
        Candle(int64_t _open, int64_t _high, int64_t _low, int64_t _close, int64_t _volume) : open(_open), high(_high), low(_low), close(_close), volume(_volume)
        {
        }

        int64_t get_open() const { return open; }

        int64_t open;
        int64_t high;
        int64_t low;
        int64_t close;
        int64_t volume;

    };
}
