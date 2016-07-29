#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

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


unsigned int DictEncObjHash(const void *key);

class HashTable
{
public:
    HashTable();
    ~HashTable();

private:
    dict* dict_;
};

#endif // HASH_TABLE_H_
