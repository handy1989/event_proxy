#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include "hash/dict.h"

#include <stdint.h>

enum HashKeyType
{
    TYPE_INT,
    TYPE_INT64,
    TYPE_CSTRING
};

struct HashKey
{
    HashKeyType type;
    union
    {
        void* ptr;
        int32_t i32;
        int64_t i64;
    } data;
};


int DictEncObjKeyCompare(void *privdata, const void *key1, const void *key2);
uint32_t DictEncObjHash(const void *key);

class HashTable
{
public:
    HashTable();
    ~HashTable();

    void* Get(HashKey* hash_key);
    bool Add(HashKey* hash_key, void* val);
    bool Delete(HashKey* hash_key);

private:
    dict* dict_;
};

#endif // HASH_TABLE_H_
