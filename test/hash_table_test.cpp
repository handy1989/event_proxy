#include "hash_table.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

uint64_t GetCurMilliseconds() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000 + now.tv_usec / 1000;
}

int main()
{
    uint64_t begin = GetCurMilliseconds();
    HashTable* hash_table = new HashTable();

    int count = 10;
    for (int i = 0; i < count; ++i)
    {
        char key[16];
        HashValue hash_value;
        sprintf(key, "hello%08d", i);
        hash_value.last_visit_time = i;

        hash_table->Add((HashKey*)key, &hash_value);
        printf("add hash_value, key:%s last_visit_time:%d\n", key, hash_value.last_visit_time);
    }
    uint64_t end = GetCurMilliseconds();
    printf("add %d items costs time %ld ms\n", count, end - begin);
    hash_table->Stat();
    begin = end;

    char key[16];
    sprintf(key, "hello%08d", 0);
    hash_table->Delete((HashKey*)key);
    printf("delete key:%s\n", key);
    
    HashValue val;
    for (int i = 0; i < 1 * count; ++i)
    {
        char key[16];
        sprintf(key, "hello%08d", i);
        if (hash_table->Get((HashKey*)key, &val))
        {
            printf("get hash_value, key:%s last_visit_time:%d\n", key, val.last_visit_time);
        }
    }
    end = GetCurMilliseconds();
    printf("query %d items costs time %ld ms\n", count, end - begin);
    hash_table->Stat();

    sleep(60);
    return 0;
}
