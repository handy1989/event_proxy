#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include "hash/dict.h"

#include <stdint.h>

typedef unsigned char HashKey;

int DictEncObjKeyCompare(void *privdata, const void *key1, void *key2);
uint32_t DictEncObjHash(const void *key);
void DictKeyDup(void* p, void *privdata, const void *key);
void DictValDup(void* p, void *privdata, const void *obj);

class HashTable
{
public:
    HashTable();
    ~HashTable();

    bool Get(HashKey* hash_key, void* val);
    bool Add(HashKey* hash_key, void* val);
    bool Delete(HashKey* hash_key);

    void Stat() {dictPrintStats(dict_);}

private:
    dict* dict_;
};

#endif // HASH_TABLE_H_
