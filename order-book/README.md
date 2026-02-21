# High-Performance C++ Order Book (Matching Engine)

A ultra-low latency limit order book and matching engine implemented in **C++20**. Designed for high-frequency trading (HFT) environments, this engine leverages **cache-coherent data structures** and **O(1) price-point indexing** to achieve sub-microsecond execution.

## 🚀 Performance Benchmarks
*Measurements taken on a 16-core 3301 MHz CPU (Ryzen 9 5000 Series / 16GB RAM).*

| Operation | Latency (Release) | Throughput |
| :--- | :--- | :--- |
| **Order Match (Execution)** | **42.2 ns** | ~23,600,000 matches/sec |
| **Limit Order Add (No Match)** | **867 ns** | ~1,150,000 orders/sec |
| **Cancel Order** | **< 100 ns** | ~10,000,000 cancels/sec |

## 🛠️ Technical Implementation

### 1. Price Point Array Indexing
The engine bypasses traditional Red-Black Tree (`std::map`) bottlenecks by using a **Flat-Array Indexing** system. By mapping prices directly to array indices, "Best Price" searches are transformed from $O(\log n)$ into **$O(1)$** pointer offsets. This ensures that matching latency remains deterministic regardless of the number of price levels.



### 2. Cache-Coherent Execution
* **Memory Management:** Utilizes a hybrid approach with `std::vector` of `std::deque` to maintain pointer stability for order tracking while providing contiguous memory blocks that maximize **L1/L2 cache hits**.
* **Index Tracking:** Employs optimized `best_bid` and `best_ask` trackers to "jump" through the price array, avoiding expensive linear scans of empty price levels.
* **Tombstone Deletion:** Cancellations utilize a "Lazy Mark" system (tombstoning), allowing $O(1)$ removals without triggering immediate memory reallocations in the hot path.

### 3. HFT Optimizations
* **Branchless Logic:** Trade quantity calculations utilize `std::min` arithmetic to reduce CPU branch mispredictions.
* **ID Lookup:** An `unordered_map` with pre-reserved buckets provides $O(1)$ access to active orders for instant cancellations.
* **Fixed-Point Precision:** Designed for high-precision assets (like BTC/USD), converting floating-point prices into integer "ticks" to eliminate rounding errors and speed up comparisons.



## 📦 Build & Test
The project includes a comprehensive GoogleTest suite and Google Benchmark integration for performance verification.

```bash
# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j

# Run Unit Tests
./order-test

# Run Micro-Benchmarks
./micro-bench
