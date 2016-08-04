#include "hash_table.h"
#include "hash/dict.h"

#include <string.h>

dictType HashDictType = {
    DictEncObjHash,             /* hash function */
    DictKeyDup,                       /* key dup */
    DictValDup,                       /* val dup */
    DictEncObjKeyCompare,       /* key compare */
    NULL, //dictRedisObjectDestructor,  /* key destructor */
    NULL //dictRedisObjectDestructor   /* val destructor */
};

void DictKeyDup(void* p, void* privdata, const void* key)
{
    HashKey* hash_key = (HashKey*)p;
    memcpy(hash_key, key, 16);
}

void DictValDup(void* p, void* privdata, const void* obj)
{
    struct DiskEntry* val = (struct DiskEntry*)p;
    memcpy(val, obj, sizeof(DiskEntry));
}

int DictEncObjKeyCompare(void *privdata, const void *key1,
        void *key2)
{
    return strncmp((char*)key1, (char*)key2, 16) == 0;
}

uint32_t DictEncObjHash(const void* hash_key) {
    return dictGenHashFunction(hash_key, 16);
}

HashTable::HashTable()
{
    dict_ = dictCreate(&HashDictType, NULL);
}

HashTable::~HashTable()
{

}

bool HashTable::Add(HashKey* hash_key, void* val)
{
    return dictAdd(dict_, hash_key, val) == DICT_OK;
}

bool HashTable::Delete(HashKey* hash_key)
{
    return dictDelete(dict_, hash_key) == DICT_OK;
}

bool HashTable::Get(HashKey* hash_key, void* val)
{
    dictEntry* de = dictFind(dict_, hash_key);
    if (de) 
    {
        memcpy(val, &de->v.val, sizeof(DiskEntry));
        return true;
    }
    return false;
}
