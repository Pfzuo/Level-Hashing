#include "level_hashing.h"

/*
Function: F_HASH()
        Compute the first hash value of a key-value item
*/
uint64_t F_HASH(level_hash *level, const uint8_t *key) {
    return (hash((void *)key, strlen(key), level->f_seed));
}

/*
Function: S_HASH() 
        Compute the second hash value of a key-value item
*/
uint64_t S_HASH(level_hash *level, const uint8_t *key) {
    return (hash((void *)key, strlen(key), level->s_seed));
}

/*
Function: F_IDX() 
        Compute the second hash location
*/
uint64_t F_IDX(uint64_t hashKey, uint64_t capacity) {
    return hashKey % (capacity / 2);
}

/*
Function: S_IDX() 
        Compute the second hash location
*/
uint64_t S_IDX(uint64_t hashKey, uint64_t capacity) {
    return hashKey % (capacity / 2) + capacity / 2;
}

/*
Function: generate_seeds() 
        Generate two randomized seeds for hash functions
*/
void generate_seeds(level_hash *level)
{
    srand(time(NULL));
    do
    {
        level->f_seed = rand();
        level->s_seed = rand();
        level->f_seed = level->f_seed << (rand() % 63);
        level->s_seed = level->s_seed << (rand() % 63);
    } while (level->f_seed == level->s_seed);
}

/*
Function: level_init() 
        Initialize a level hash table
*/
level_hash *level_init(uint64_t level_size)
{
    level_hash *level = malloc(sizeof(level_hash));
    if (!level)
    {
        printf("The level hash table initialization fails:1\n");
        exit(1);
    }

    level->level_size = level_size;
    level->addr_capacity = pow(2, level_size);
    level->total_capacity = pow(2, level_size) + pow(2, level_size - 1);
    level->buckets[0] = calloc(pow(2, level_size), sizeof(level_bucket));
    level->buckets[1] = calloc(pow(2, level_size - 1), sizeof(level_bucket));
    level->level_locks[0] = calloc(pow(2, level_size), sizeof(level_locks));
    level->level_locks[1] = calloc(pow(2, level_size - 1), sizeof(level_locks));

    generate_seeds(level);
    level->level_resize = 0;
    
    if (!level->buckets[0] || !level->buckets[1])
    {
        printf("The level hash table initialization fails:2\n");
        exit(1);
    }

    printf("Level hashing: ASSOC_NUM %d, KEY_LEN %d, VALUE_LEN %d \n", ASSOC_NUM, KEY_LEN, VALUE_LEN);
    printf("The number of top-level buckets: %ld\n", level->addr_capacity);
    printf("The number of all buckets: %ld\n", level->total_capacity);
    printf("The number of all entries: %ld\n", level->total_capacity*ASSOC_NUM);
    printf("The level hash table initialization succeeds!\n");
    return level;
}

/*
Function: level_resize()
        Expand a level hash table in place;
        Put a new level on the top of the old hash table and only rehash the
        items in the bottom level of the old hash table;
*/
void level_resize(level_hash *level) 
{
    if (!level)
    {
        printf("The resizing fails: 1\n");
        exit(1);
    }

    level->addr_capacity = pow(2, level->level_size + 1);
    level_bucket *newBuckets = calloc(level->addr_capacity, sizeof(level_bucket));
    if (!newBuckets) {
        printf("The resizing fails: 2\n");
        exit(1);
    }
    
    uint64_t old_idx;
    for (old_idx = 0; old_idx < pow(2, level->level_size - 1); old_idx ++) {
        uint64_t i, j;
        for(i = 0; i < ASSOC_NUM; i ++){
            if (level->buckets[1][old_idx].token[i] == 1)
            {
                uint8_t *key = level->buckets[1][old_idx].slot[i].key;
                uint8_t *value = level->buckets[1][old_idx].slot[i].value;

                uint64_t f_idx = F_IDX(F_HASH(level, key), level->addr_capacity);
                uint64_t s_idx = S_IDX(S_HASH(level, key), level->addr_capacity);

                uint8_t insertSuccess = 0;
                for(j = 0; j < ASSOC_NUM; j ++){                            
                    /*  The rehashed item is inserted into the less-loaded bucket between 
                        the two hash locations in the new level
                    */
                    if (newBuckets[f_idx].token[j] == 0)
                    {
                        memcpy(newBuckets[f_idx].slot[j].key, key, KEY_LEN);
                        memcpy(newBuckets[f_idx].slot[j].value, value, VALUE_LEN);
                        newBuckets[f_idx].token[j] = 1;
                        insertSuccess = 1;

                        break;
                    }
                    if (newBuckets[s_idx].token[j] == 0)
                    {
                        memcpy(newBuckets[s_idx].slot[j].key, key, KEY_LEN);
                        memcpy(newBuckets[s_idx].slot[j].value, value, VALUE_LEN);
                        newBuckets[s_idx].token[j] = 1;
                        insertSuccess = 1;

                        break;
                    }
                }
                if(!insertSuccess){
                    printf("The resizing fails: 3\n");
                    exit(1);                    
                }
				
				level->buckets[1][old_idx].token[i] = 0;
            }
        }
    }

    level->level_size ++;
    level->total_capacity = pow(2, level->level_size) + pow(2, level->level_size - 1);

    free(level->buckets[1]);
    level->buckets[1] = level->buckets[0];
    level->buckets[0] = newBuckets;
    newBuckets = NULL;

    level->level_resize ++;
}


/*
Function: level_query() 
        Lookup a key-value item in level hash table;
*/
uint8_t level_query(level_hash *level, uint8_t *key, uint8_t *value)
{
    uint64_t f_hash = F_HASH(level, key);
    uint64_t s_hash = S_HASH(level, key);
    uint64_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint64_t s_idx = S_IDX(s_hash, level->addr_capacity);
    
    uint64_t i, j;
    for(i = 0; i < 2; i ++){
        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[i][f_idx].s_lock[j]);
            if (level->buckets[i][f_idx].token[j] == 1&&strcmp(level->buckets[i][f_idx].slot[j].key, key) == 0)
            {
                memcpy(value, level->buckets[i][f_idx].slot[j].value, VALUE_LEN);
                spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
        }
        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[i][s_idx].s_lock[j]);
            if (level->buckets[i][s_idx].token[j] == 1&&strcmp(level->buckets[i][s_idx].slot[j].key, key) == 0)
            {    
                memcpy(value, level->buckets[i][s_idx].slot[j].value, VALUE_LEN);
                spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
        }
        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }

    return 1;
}


/*
Function: level_delete() 
        Remove a key-value item from level hash table;
*/
uint8_t level_delete(level_hash *level, uint8_t *key)
{
    uint64_t f_hash = F_HASH(level, key);
    uint64_t s_hash = S_HASH(level, key);
    uint64_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint64_t s_idx = S_IDX(s_hash, level->addr_capacity);
    
    uint64_t i, j;
    for(i = 0; i < 2; i ++){
        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[i][f_idx].s_lock[j]);
            if (level->buckets[i][f_idx].token[j] == 1&&strcmp(level->buckets[i][f_idx].slot[j].key, key) == 0)
            {
                level->buckets[i][f_idx].token[j] = 0;
                spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
        }
        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[i][s_idx].s_lock[j]);
            if (level->buckets[i][s_idx].token[j] == 1&&strcmp(level->buckets[i][s_idx].slot[j].key, key) == 0)
            {
                level->buckets[i][s_idx].token[j] = 0;
                spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
        }
        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }

    return 1;
}

/*
Function: level_update() 
        Update the value of a key-value item in level hash table;
        The function can be optimized by using the dynamic search scheme
*/
uint8_t level_update(level_hash *level, uint8_t *key, uint8_t *new_value)
{
    uint64_t f_hash = F_HASH(level, key);
    uint64_t s_hash = S_HASH(level, key);
    uint64_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint64_t s_idx = S_IDX(s_hash, level->addr_capacity);
    
    uint64_t i, j;
    for(i = 0; i < 2; i ++){
        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[i][f_idx].s_lock[j]);
            if (level->buckets[i][f_idx].token[j] == 1&&strcmp(level->buckets[i][f_idx].slot[j].key, key) == 0)
            {
                memcpy(level->buckets[i][f_idx].slot[j].value, new_value, VALUE_LEN);
                spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
        }
        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[i][s_idx].s_lock[j]);
            if (level->buckets[i][s_idx].token[j] == 1&&strcmp(level->buckets[i][s_idx].slot[j].key, key) == 0)
            {
                memcpy(level->buckets[i][s_idx].slot[j].value, new_value, VALUE_LEN);
                spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
        }
        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }

    return 1;
}

/*
Function: level_insert() 
        Insert a key-value item into level hash table;
*/
uint8_t level_insert(level_hash *level, uint8_t *key, uint8_t *value)
{
    uint64_t f_hash = F_HASH(level, key);
    uint64_t s_hash = S_HASH(level, key);
    uint64_t f_idx = F_IDX(f_hash, level->addr_capacity);
    uint64_t s_idx = S_IDX(s_hash, level->addr_capacity);

    uint64_t i, j;
    int empty_location;

    for(i = 0; i < 2; i ++){
        for(j = 0; j < ASSOC_NUM; j ++){        
            /*  The new item is inserted into the less-loaded bucket between 
                the two hash locations in each level           
            */		
            spin_lock(&level->level_locks[i][f_idx].s_lock[j]);
            if (level->buckets[i][f_idx].token[j] == 0)
            {
                memcpy(level->buckets[i][f_idx].slot[j].key, key, KEY_LEN);
                memcpy(level->buckets[i][f_idx].slot[j].value, value, VALUE_LEN);
                level->buckets[i][f_idx].token[j] = 1;
                spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][f_idx].s_lock[j]);
            spin_lock(&level->level_locks[i][s_idx].s_lock[j]);
            if (level->buckets[i][s_idx].token[j] == 0) 
            {
                memcpy(level->buckets[i][s_idx].slot[j].key, key, KEY_LEN);
                memcpy(level->buckets[i][s_idx].slot[j].value, value, VALUE_LEN);
                level->buckets[i][s_idx].token[j] = 1;
                spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
                return 0;
            }
            spin_unlock(&level->level_locks[i][s_idx].s_lock[j]);
        }

        f_idx = F_IDX(f_hash, level->addr_capacity / 2);
        s_idx = S_IDX(s_hash, level->addr_capacity / 2);
    }

    f_idx = F_IDX(f_hash, level->addr_capacity);
    s_idx = S_IDX(s_hash, level->addr_capacity);
    
    for(i = 0; i < 2; i++){
        if(!try_movement(level, f_idx, i, key, value)){
            return 0;
        }
        if(!try_movement(level, s_idx, i, key, value)){
            return 0;
        }

        f_idx = F_IDX(f_hash, level->addr_capacity/2);
        s_idx = S_IDX(s_hash, level->addr_capacity/2);        
    }

    if(level->level_resize > 0){
        empty_location = b2t_movement(level, f_idx);
        if(empty_location != -1){
            memcpy(level->buckets[1][f_idx].slot[empty_location].key, key, KEY_LEN);
            memcpy(level->buckets[1][f_idx].slot[empty_location].value, value, VALUE_LEN);
            level->buckets[1][f_idx].token[empty_location] = 1;
            spin_unlock(&level->level_locks[1][f_idx].s_lock[empty_location]);
            return 0;
        }

        empty_location = b2t_movement(level, s_idx);
        if(empty_location != -1){
            memcpy(level->buckets[1][s_idx].slot[empty_location].key, key, KEY_LEN);
            memcpy(level->buckets[1][s_idx].slot[empty_location].value, value, VALUE_LEN);
            level->buckets[1][s_idx].token[empty_location] = 1;
            spin_unlock(&level->level_locks[1][s_idx].s_lock[empty_location]);
            return 0;
        }
    }

    return 1;
}

/*
Function: try_movement() 
        Try to move an item from the current bucket to its same-level alternative bucket;
*/
uint8_t try_movement(level_hash *level, uint64_t idx, uint64_t level_num, uint8_t *key, uint8_t *value)
{
    uint64_t i, j, jdx;

    for(i = 0; i < ASSOC_NUM; i ++){
        spin_lock(&level->level_locks[level_num][idx].s_lock[i]);
        uint8_t *m_key = level->buckets[level_num][idx].slot[i].key;
        uint8_t *m_value = level->buckets[level_num][idx].slot[i].value;
        uint64_t f_hash = F_HASH(level, m_key);
        uint64_t s_hash = S_HASH(level, m_key);
        uint64_t f_idx = F_IDX(f_hash, level->addr_capacity/(1+level_num));
        uint64_t s_idx = S_IDX(s_hash, level->addr_capacity/(1+level_num));
        
        if(f_idx == idx)
            jdx = s_idx;
        else
            jdx = f_idx;

        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[level_num][jdx].s_lock[j]);
            if (level->buckets[level_num][jdx].token[j] == 0)
            {
                memcpy(level->buckets[level_num][jdx].slot[j].key, m_key, KEY_LEN);
                memcpy(level->buckets[level_num][jdx].slot[j].value, m_value, VALUE_LEN);
                level->buckets[level_num][jdx].token[j] = 1;
                level->buckets[level_num][idx].token[i] = 0;
                spin_unlock(&level->level_locks[level_num][jdx].s_lock[j]);
                // The movement is finished and then the new item is inserted

                memcpy(level->buckets[level_num][idx].slot[i].key, key, KEY_LEN);
                memcpy(level->buckets[level_num][idx].slot[i].value, value, VALUE_LEN);
                level->buckets[level_num][idx].token[i] = 1;
                spin_unlock(&level->level_locks[level_num][idx].s_lock[i]);  

                return 0;
            }
            spin_unlock(&level->level_locks[level_num][jdx].s_lock[j]);
        }
        spin_unlock(&level->level_locks[level_num][idx].s_lock[i]);        
    }
    
    return 1;
}

/*
Function: b2t_movement() 
        Try to move a bottom-level item to its top-level alternative buckets;
*/
int b2t_movement(level_hash *level, uint64_t idx)
{
    uint8_t *key, *value;
    uint64_t s_hash, f_hash;
    uint64_t s_idx, f_idx;
    
    uint64_t i, j;
    for(i = 0; i < ASSOC_NUM; i ++){
        spin_lock(&level->level_locks[1][idx].s_lock[i]);
        key = level->buckets[1][idx].slot[i].key;
        value = level->buckets[1][idx].slot[i].value;
        f_hash = F_HASH(level, key);
        s_hash = S_HASH(level, key);  
        f_idx = F_IDX(f_hash, level->addr_capacity);
        s_idx = S_IDX(s_hash, level->addr_capacity);
    
        for(j = 0; j < ASSOC_NUM; j ++){
            spin_lock(&level->level_locks[0][f_idx].s_lock[j]);
            if (level->buckets[0][f_idx].token[j] == 0)
            {
                memcpy(level->buckets[0][f_idx].slot[j].key, key, KEY_LEN);
                memcpy(level->buckets[0][f_idx].slot[j].value, value, VALUE_LEN);
                level->buckets[0][f_idx].token[j] = 1;
                level->buckets[1][idx].token[i] = 0;
                spin_unlock(&level->level_locks[0][f_idx].s_lock[j]);
                return i;
            }
            spin_unlock(&level->level_locks[0][f_idx].s_lock[j]);
            spin_lock(&level->level_locks[0][s_idx].s_lock[j]);
            if (level->buckets[0][s_idx].token[j] == 0)
            {
                memcpy(level->buckets[0][s_idx].slot[j].key, key, KEY_LEN);
                memcpy(level->buckets[0][s_idx].slot[j].value, value, VALUE_LEN);
                level->buckets[0][s_idx].token[j] = 1;
                level->buckets[1][idx].token[i] = 0;
                spin_unlock(&level->level_locks[0][s_idx].s_lock[j]);
                return i;
            }
            spin_unlock(&level->level_locks[0][s_idx].s_lock[j]);
        }
        spin_unlock(&level->level_locks[1][idx].s_lock[i]);
    }

    return -1;
}

/*
Function: level_destroy() 
        Destroy a level hash table
*/
void level_destroy(level_hash *level)
{
    free(level->buckets[0]);
    free(level->buckets[1]);
    level = NULL;
}


void ycsb_thread_run(void* arg){
    sub_thread* subthread = arg;
    uint8_t key[KEY_LEN];
    uint8_t value[VALUE_LEN]; 
    int i = 0;
    printf("Thread %d is opened\n", subthread->id);
    for(; i < READ_WRITE_NUM/subthread->level->thread_num; i++){
        if( subthread->run_queue[i].operation == 1){
            if (!level_insert(subthread->level, subthread->run_queue[i].key, subthread->run_queue[i].key)){   
                subthread->inserted ++;
            }
        }else{
            if(!level_query(subthread->level, subthread->run_queue[i].key, value))
                // Get value
                ;
        }
    }
    pthread_exit(NULL);
}