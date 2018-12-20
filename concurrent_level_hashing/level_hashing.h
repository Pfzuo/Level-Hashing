#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include "hash.h"
#include "spinlock.h"

#define ASSOC_NUM 4                       // The number of slots in a bucket
#define KEY_LEN 16                        // The maximum length of a key
#define VALUE_LEN 15                      // The maximum length of a value
#define READ_WRITE_NUM 350000             // The total number of read and write operations in the workload

typedef struct entry{                     // A slot storing a key-value item 
    uint8_t key[KEY_LEN];
    uint8_t value[VALUE_LEN];
} entry;

typedef struct level_bucket               // A bucket
{
    uint8_t token[ASSOC_NUM];             // A token indicates whether its corresponding slot is empty, which can also be implemented using 1 bit
    entry slot[ASSOC_NUM];
} level_bucket;

typedef struct level_locks{
    spinlock s_lock[ASSOC_NUM];
} level_locks;

typedef struct level_hash {               // A Level hash table
    level_bucket *buckets[2];             // The top level and bottom level in the Level hash table
    level_locks* level_locks[2];          // Allocate a fine-grained lock for each slot

    uint32_t thread_num;
    uint64_t addr_capacity;               // The number of buckets in the top level
    uint64_t total_capacity;              // The number of all buckets in the Level hash table    
    uint64_t level_size;                  // level_size = log2(addr_capacity)
    uint8_t level_resize;                 // Indicate whether the Level hash table was resized, "1": Yes, "0": No;
    uint64_t f_seed;
    uint64_t s_seed;                      // Two randomized seeds for hash functions
} level_hash;

typedef struct thread_queue{
    uint8_t key[KEY_LEN];
    uint8_t operation;                    // 0: read, 1: insert;
} thread_queue;

typedef struct sub_thread{
    pthread_t thread;
    uint32_t id;
    uint64_t inserted;
    level_hash *level;
    thread_queue* run_queue;
} sub_thread;


level_hash *level_init(uint64_t level_size);     

uint8_t level_insert(level_hash *level, uint8_t *key, uint8_t *value);          

uint8_t level_query(level_hash *level, uint8_t *key, uint8_t *value);

uint8_t level_delete(level_hash *level, uint8_t*key);

uint8_t level_update(level_hash *level, uint8_t *key, uint8_t *new_value);

void level_resize(level_hash *level);

uint8_t try_movement(level_hash *level, uint64_t idx, uint64_t level_num, uint8_t *key, uint8_t *value);

int b2t_movement(level_hash *level, uint64_t idx);

void level_destroy(level_hash *level);

void ycsb_thread_run(void* arg);