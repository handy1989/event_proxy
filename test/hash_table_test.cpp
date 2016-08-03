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
    int count = 1024 * 1024;
    int64_t total_key_size = 0;
    int64_t total_value_size = 0;
    for (int i = 0; i < count; ++i)
    {
        HashKey* hash_key = new HashKey();
        char* key = (char*)malloc(20);
        char* value = (char*)malloc(20);
        sprintf(key, "hello%08d", i);
        sprintf(value, "world%08d", i);
        hash_key->type = TYPE_CSTRING;
        hash_key->data.ptr = (void*)key;
        hash_table->Add(hash_key, value);
        //printf("add key:%s value:%s\n", (char*)hash_key->data.ptr, value);
    }
    uint64_t end = GetCurMilliseconds();
    printf("add %d items costs time %ld ms\n", count, end - begin);
    hash_table->Stat();
    //begin = end;
    //HashKey* hash_key = new HashKey();
    //char* key = (char*)malloc(30);
    //sprintf(key, "hello%08d", 0);
    //hash_key->type = TYPE_CSTRING;
    //hash_key->data.ptr = (void*)key;
    //for (int i = 0; i < 1 * count; ++i)
    //{
    //    hash_table->Get(hash_key);
    //}
    //end = GetCurMilliseconds();
    //printf("query %d items costs time %ld ms\n", count, end - begin);
    //hash_table->Stat();

    sleep(60);
    return 0;
}
