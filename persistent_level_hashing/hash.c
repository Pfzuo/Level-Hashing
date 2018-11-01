#include "hash.h"

#define NUMBER64_1 11400714785074694791ULL
#define NUMBER64_2 14029467366897019727ULL
#define NUMBER64_3 1609587929392839161ULL
#define NUMBER64_4 9650029242287828579ULL
#define NUMBER64_5 2870177450012600261ULL

#define hash_get64bits(x) hash_read64_align(x, align)
#define hash_get32bits(x) hash_read32_align(x, align)
#define shifting_hash(x, r) ((x << r) | (x >> (64 - r)))
#define TO64(x) (((U64_INT *)(x))->v)
#define TO32(x) (((U32_INT *)(x))->v)


typedef struct U64_INT
{
    uint64_t v;
} U64_INT;

typedef struct U32_INT
{
    uint32_t v;
} U32_INT;

uint64_t hash_read64_align(const void *ptr, uint32_t align)
{
    if (align == 0)
    {
        return TO64(ptr);
    }
    return *(uint64_t *)ptr;
}

uint32_t hash_read32_align(const void *ptr, uint32_t align)
{
    if (align == 0)
    {
        return TO32(ptr);
    }
    return *(uint32_t *)ptr;
}

/*
Function: string_key_hash_computation() 
        A hash function for string keys
*/
uint64_t string_key_hash_computation(const void *data, uint64_t length, uint64_t seed, uint32_t align)
{
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *end = p + length;
    uint64_t hash;

    if (length >= 32)
    {
        const uint8_t *const limitation = end - 32;
        uint64_t v1 = seed + NUMBER64_1 + NUMBER64_2;
        uint64_t v2 = seed + NUMBER64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - NUMBER64_1;

        do
        {
            v1 += hash_get64bits(p) * NUMBER64_2;
            p += 8;
            v1 = shifting_hash(v1, 31);
            v1 *= NUMBER64_1;
            v2 += hash_get64bits(p) * NUMBER64_2;
            p += 8;
            v2 = shifting_hash(v2, 31);
            v2 *= NUMBER64_1;
            v3 += hash_get64bits(p) * NUMBER64_2;
            p += 8;
            v3 = shifting_hash(v3, 31);
            v3 *= NUMBER64_1;
            v4 += hash_get64bits(p) * NUMBER64_2;
            p += 8;
            v4 = shifting_hash(v4, 31);
            v4 *= NUMBER64_1;
        } while (p <= limitation);

        hash = shifting_hash(v1, 1) + shifting_hash(v2, 7) + shifting_hash(v3, 12) + shifting_hash(v4, 18);

        v1 *= NUMBER64_2;
        v1 = shifting_hash(v1, 31);
        v1 *= NUMBER64_1;
        hash ^= v1;
        hash = hash * NUMBER64_1 + NUMBER64_4;

        v2 *= NUMBER64_2;
        v2 = shifting_hash(v2, 31);
        v2 *= NUMBER64_1;
        hash ^= v2;
        hash = hash * NUMBER64_1 + NUMBER64_4;

        v3 *= NUMBER64_2;
        v3 = shifting_hash(v3, 31);
        v3 *= NUMBER64_1;
        hash ^= v3;
        hash = hash * NUMBER64_1 + NUMBER64_4;

        v4 *= NUMBER64_2;
        v4 = shifting_hash(v4, 31);
        v4 *= NUMBER64_1;
        hash ^= v4;
        hash = hash * NUMBER64_1 + NUMBER64_4;
    }
    else
    {
        hash = seed + NUMBER64_5;
    }

    hash += (uint64_t)length;

    while (p + 8 <= end)
    {
        uint64_t k1 = hash_get64bits(p);
        k1 *= NUMBER64_2;
        k1 = shifting_hash(k1, 31);
        k1 *= NUMBER64_1;
        hash ^= k1;
        hash = shifting_hash(hash, 27) * NUMBER64_1 + NUMBER64_4;
        p += 8;
    }

    if (p + 4 <= end)
    {
        hash ^= (uint64_t)(hash_get32bits(p)) * NUMBER64_1;
        hash = shifting_hash(hash, 23) * NUMBER64_2 + NUMBER64_3;
        p += 4;
    }

    while (p < end)
    {
        hash ^= (*p) * NUMBER64_5;
        hash = shifting_hash(hash, 11) * NUMBER64_1;
        p++;
    }

    hash ^= hash >> 33;
    hash *= NUMBER64_2;
    hash ^= hash >> 29;
    hash *= NUMBER64_3;
    hash ^= hash >> 32;

    return hash;
}

uint64_t hash(const void *data, uint64_t length, uint64_t seed)
{
    if ((((uint64_t)data) & 7) == 0)
    {
        return string_key_hash_computation(data, length, seed, 1);
    }
    return string_key_hash_computation(data, length, seed, 0);
}
