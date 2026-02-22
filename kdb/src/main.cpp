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


int main()
{

}
