#include <stdint.h>

/*
Function: hash() 
        This function is used to compute the hash value of a string key;
        For integer keys, two different and independent hash functions should be used. 
        For example, Jenkins Hash is used for the first hash funciton, and murmur3 hash is used for
        the second hash funciton.
*/
uint64_t hash(const void *data, uint64_t length, uint64_t seed);


