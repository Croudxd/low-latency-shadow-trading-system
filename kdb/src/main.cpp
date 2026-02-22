/**
 *
 * Okay the way this will work to make sure we are not too slow and dont slow down our order-book.
 *
 * We will ingest data from a the SPSCs that backtester has with the order-book, since we can just do a lossy SPSC, the logger will be faster
 * than the strategy. 
 *
 * We will then add that to another SPSC / DS and have logger B take that data and send to a kdb+, this way we dont slow the order-book or strategy,
 * but maintain most if not all the data.
 *
 * Plus its ethereum and it quite slow (other coins can be faster but i think this will handle it). In terms of a options/fast stock this might
 * stuggle. 
 */


#include <iostream>
#include "q/k.h" 

int main() {
    std::cout << "Attempting to connect to kdb+ on port 5001..." << std::endl;

    // Connect to localhost on port 5001 (timeout usually optional, but good practice)
    // khpu(hostname, port, "username:password") - we leave credentials blank for now
    int handle = khpu((char*)"localhost", 5001, (char*)"");

    if (handle <= 0) {
        std::cerr << "Failed to connect! Is kdb+ running on port 5001?" << std::endl;
        return 1;
    }

    std::cout << "Connected successfully! Handle ID: " << handle << std::endl;

    // Send a synchronous request to the kdb+ server to calculate 10 + 20
    // The (K)0 at the end tells kdb+ there are no more arguments
    K result = k(handle, (char*)"10 + 20", (K)0);

    if (!result) {
        std::cerr << "Network error during communication." << std::endl;
    } else if (result->t == -128) { // -128 is the kdb+ error type
        std::cerr << "kdb+ Server Error: " << result->s << std::endl;
        r0(result); // Free the memory
    } else {
        // ->j is the accessor for a 'long' integer in k.h
        std::cout << "Success! kdb+ calculated: 10 + 20 = " << result->j << std::endl;
        r0(result); // Always free the result object when done!
    }

    // Close the connection
    kclose(handle);
    std::cout << "Connection closed." << std::endl;

    return 0;
}
