# strategy

## Build

Either use the `run.sh` script which will build and run all components needed.

or

```
mkdir build && cd build
cmake ..                  [release flags if wanted]
make -j
./strategy
```

Will wait for the order-book and viewer before doing anything.


### Example
```C++
#include "Engine.hpp"
#include "indicator.hpp"
#include "ring_buffer.hpp"
#include "sma.hpp"
// Strategy class is needed, You must create this class and put your strategy in the run function:
// void run(backtester::Ring_buffer& ring_buffer, backtester::Engine<Strategy>& engine) else the engine will not run your strategy.

class Strategy
{
    backtester::SMA short_ma { 6 };
    backtester::SMA long_ma { 12 };

    public:
        void run(backtester::Ring_buffer& ring_buffer, backtester::Engine<Strategy>& engine)
        {
            double current_price = (double)ring_buffer.get(0).get_open();

            long sma_s = short_ma.calculate(); 
            long sma_l = long_ma.calculate();

            if (sma_s > sma_l) 
            {
                engine.order(1.0, current_price, backtester::Order_side::BUY, false);
            }
        }
};

int main()
{
    backtester::Engine<Strategy> engine(100000000.0, 0.000001);
    backtester::Indicator::set_ring_buffer(engine.get_ring_buffer());
    engine.connect();
    engine.warmup(24);
    engine.run();
    return 0;
}
```

### Engine Class

Constructor

```C++
//Takes two doubles for starting cash and fees (ie 10000, 0.001)

Engine(double starting_cash, double fees) : portfolio(starting_cash, fees);

```

```C++
// Takes a char path and MemoryFlags
// Will return a pointer to our spsc memory.

template <typename T> 
T* map_mem(const char* path, MemoryFlags flag)

```
```C++
// Will create all memory maps needed to send/recieve data from order-book.

void connect()

```

```C++
// Will start running the engine,
// If you have set a warmup, it will loop until it has the amount of data wanted.

void run();

```


```C++
// order function, used for placing orders back to the order-book.
// Order book will return a Report struct after every order is sent.

void order(float size, float price, Order_side side)

```

```C++
// Will cancel and order given the ID;

void cancel_order(uint64_t target_id)

```

```C++
// Will stall the engine until size is equal to the amount of candles the backtester has in storage.
// I suggest you to warm up the the engine to your maximum indicator amount. ie SMA 10 warmup should be 10.

void warmup(size_t size)
```

```C++
// Function takes a Report object reference,
// Will update portfolio and active orders / history based on the input.

void on_report(const Report& raw)
```
```C+++


```
### Indicator


```C++
// This should be used to create indicators, (also custom indiactors can be implemented buy should inherit from this class).
// See how it is used in the example about, but you MUST pass the ring buffer to it, or you will seg fault the program.

    class Indicator 
    {
        public:
            static void set_ring_buffer(backtester::Ring_buffer* _ring_buffer)
            {
                ring_buffer = _ring_buffer;
            }
        protected:
            inline static backtester::Ring_buffer* ring_buffer = nullptr;
```

#### SMA

```C++
// In our main file, We can override the Strategy class, however you must make sure it has a run function.
// Inside that class (again see example) sma can be initalized, this will make it possible to calculate the SMA.

SMA(size_t size) : period(size) {}
// This will initzalize the object,
// You will need to call the calculate() function for it to update.
// The engine has a loop so only need to call this once.

backtester::SMA long_ma { 12 };

// In on_run()
long_ma.calculate();
```

```C++
// Ring buffer can be called, only stores Candle objects.
// Has api for add(), get(), and size().

Ring_buffer () = default;

void add(const Candle& item)

const Candle& get(size_t index) const

size_t size() const     
```
