#include "log.h"

/*
Function: log_create() 
        Create a log;
*/
level_log* log_create(uint64_t log_length)
{
    level_log* log = pmalloc(sizeof(level_log));
    if (!log)
    {
        printf("Log creation fails: 1\n");
        exit(1);
    }

    log->entry = pmalloc(log_length*sizeof(log_entry));
    if (!log->entry)
    {
        printf("Log creation fails: 2");
        exit(1);
    }

    log->log_length = log_length;
    log->current = 0;

    return log;
}

/*
Function: log_write() 
        Write a log entry;
*/
void log_write(level_log *log, uint8_t *key, uint8_t *value)
{
    memcpy(log->entry[log->current].key, key, KEY_LEN);
    memcpy(log->entry[log->current].value, value, VALUE_LEN);
    pflush((uint64_t *)&log->entry[log->current].key);
    pflush((uint64_t *)&log->entry[log->current].value);
    asm_mfence();
    
    log->entry[log->current].flag = 1;
    pflush((uint64_t *)&log->entry[log->current].flag);
    asm_mfence();
}

/*
Function: log_clean() 
        Clean up a log entry;
*/
void log_clean(level_log *log)
{
    log->entry[log->current].flag = 0;
    pflush((uint64_t *)&log->entry[log->current].flag);
    asm_mfence();

    log->current ++;
    if(log->current == log->log_length)
        log->current = 0;
    pflush((uint64_t *)&log->current);
    asm_mfence();
}