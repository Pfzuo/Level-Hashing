#include "level_hashing.h"

/*  Test:
    This is a simple test example to test the creation, insertion, search, deletion, update in Level hashing
*/
int main(int argc, char* argv[])                        
{
    int level_size = atoi(argv[1]);                     // INPUT: the number of addressable buckets is 2^level_size
    int insert_num = atoi(argv[2]);                     // INPUT: the number of items to be inserted
    int write_latency = atoi(argv[3]);                  // INPUT: the injected write latency
    
    init_pflush(2000, write_latency);
    level_hash *level = level_init(level_size);
    uint64_t inserted = 0, i = 0;
    uint8_t key[KEY_LEN];
    uint8_t value[VALUE_LEN];

    for (i = 1; i < insert_num + 1; i ++)
    {
        snprintf(key, KEY_LEN, "%ld", i);
        snprintf(value, VALUE_LEN, "%ld", i);
        if (!level_insert(level, key, value))                               
        {
            inserted ++;
        }else
        {
            printf("Expanding: space utilization & total entries: %f  %ld\n", \
                (float)(level->level_item_num[0]+level->level_item_num[1])/(level->total_capacity*ASSOC_NUM), \
                level->total_capacity*ASSOC_NUM);
            level_expand(level);
            level_insert(level, key, value);
            inserted ++;
        }
    }   
    printf("%ld items are inserted\n", inserted);

    printf("The static search test begins ...\n");
    for (i = 1; i < insert_num + 1; i ++)
    {
        snprintf(key, KEY_LEN, "%ld", i);
        uint8_t* get_value = level_static_query(level, key);
        if(get_value == NULL)
            printf("Search the key %s: ERROR! \n", key);
   }

    printf("The dynamic search test begins ...\n");
    for (i = 1; i < insert_num + 1; i ++)
    {
        snprintf(key, KEY_LEN, "%ld", i);
        uint8_t* get_value = level_dynamic_query(level, key);
        if(get_value == NULL)
            printf("Search the key %s: ERROR! \n", key);
   }

    printf("The update test begins ...\n");
    for (i = 1; i < insert_num + 1; i ++)
    {
        snprintf(key, KEY_LEN, "%ld", i);
        snprintf(value, VALUE_LEN, "%ld", i*2);
        if(level_update(level, key, value))
            printf("Update the value of the key %s: ERROR! \n", key);
   }

    printf("The deletion test begins ...\n");
    for (i = 1; i < insert_num + 1; i ++)
    {
        snprintf(key, KEY_LEN, "%ld", i);
        if(level_delete(level, key))
            printf("Delete the key %s: ERROR! \n", key);
   }

    printf("The number of items stored in the level hash table: %ld\n", level->level_item_num[0]+level->level_item_num[1]);    
    level_destroy(level);

    return 0;
}
