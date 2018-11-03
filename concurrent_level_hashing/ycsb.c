#include "level_hashing.h"

/*  YCSB test:
    This is a simple test example to test the concurrent level hashing 
    using a YCSB workload with 50%/50% search/insertion ratio
*/
int main(int argc, char* argv[])
{        
    int thread_num = atoi(argv[1]);             // INPUT: the number of threads

    level_hash *level = level_init(19);
    level->thread_num = thread_num;
    uint64_t inserted = 0, queried = 0, t = 0;
    uint8_t key[KEY_LEN];
    uint8_t value[VALUE_LEN];

	FILE *ycsb, *ycsb_read;
	char *buf = NULL;
	size_t len = 0;
    struct timespec start, finish;
    double single_time;

    if((ycsb = fopen("./workloads/rw-50-50-load.txt","r")) == NULL)
    {
        perror("fail to read");
    }

    printf("Load phase begins \n");
	while(getline(&buf,&len,ycsb) != -1){
		if(strncmp(buf, "INSERT", 6) == 0){
			memcpy(key, buf+7, KEY_LEN-1);
			if (!level_insert(level, key, key))                      
			{
				inserted ++;
			}
			else{
				break;
			}
		}
	}
	fclose(ycsb);
    printf("Load phase finishes: %d items are inserted \n", inserted);

    if((ycsb_read = fopen("./workloads/rw-50-50-run.txt","r")) == NULL)
    {
        perror("fail to read");
    }

    thread_queue* run_queue[thread_num];
    int move[thread_num];
    for(t = 0; t < thread_num; t ++){
        run_queue[t] =  calloc(READ_WRITE_NUM/thread_num, sizeof(thread_queue));
        move[t] = 0;
    }

	int operation_num = 0;		
	while(getline(&buf,&len,ycsb_read) != -1){
		if(strncmp(buf, "INSERT", 6) == 0){
			memcpy(run_queue[operation_num%thread_num][move[operation_num%thread_num]].key, buf+7, KEY_LEN-1);
			run_queue[operation_num%thread_num][move[operation_num%thread_num]].operation = 1;
			move[operation_num%thread_num] ++;
		}
		else if(strncmp(buf, "READ", 4) == 0){
			memcpy(run_queue[operation_num%thread_num][move[operation_num%thread_num]].key, buf+5, KEY_LEN-1);
			run_queue[operation_num%thread_num][move[operation_num%thread_num]].operation = 0;
			move[operation_num%thread_num] ++;
		}
		operation_num ++;
	}
	fclose(ycsb_read);

	sub_thread* THREADS = (sub_thread*)malloc(sizeof(sub_thread)*thread_num);
    inserted = 0;
	
	printf("Run phase begins: SEARCH/INSERTION ratio is 50\%:50\% \n");
    clock_gettime(CLOCK_MONOTONIC, &start);	
    for(t = 0; t < thread_num; t++){
        THREADS[t].id = t;
        THREADS[t].level = level;
        THREADS[t].inserted = 0;
        THREADS[t].run_queue = run_queue[t];
        pthread_create(&THREADS[t].thread, NULL, (void *)ycsb_thread_run, &THREADS[t]);
    }

    for(t = 0; t < thread_num; t++){
        pthread_join(THREADS[t].thread, NULL);
    }

	clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    for(t = 0; t < thread_num; ++t){
        inserted +=  THREADS[t].inserted;
    }
    printf("Run phase finishes: %d/%d items are inserted/searched\n", operation_num - inserted, inserted);
    printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);	
    
    return 0;
}
