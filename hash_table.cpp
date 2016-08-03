#include "hash_table.h"
#include "hash/dict.h"

#include <string.h>

dictType HashDictType = {
    DictEncObjHash,             /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    DictEncObjKeyCompare,       /* key compare */
    NULL, //dictRedisObjectDestructor,  /* key destructor */
    NULL //dictRedisObjectDestructor   /* val destructor */
};

int DictEncObjKeyCompare(void *privdata, const void *key1,
        const void *key2)
{
    HashKey* hash_key1 = (HashKey*)key1;
    HashKey* hash_key2 = (HashKey*)key2;
    if (hash_key1->type != hash_key2->type) return 0;
    switch (hash_key1->type)
    {
        case TYPE_INT:
            return hash_key1->data.i32 == hash_key2->data.i32;
            break;
        case TYPE_INT64:
            return hash_key1->data.i64 == hash_key2->data.i64;
            break;
        case TYPE_CSTRING:
            return strcmp((char*)hash_key1->data.ptr, (char*)hash_key2->data.ptr) == 0;
            break;
        default:
            return 0;
    }
    return 0;
}

uint32_t DictEncObjHash(const void *key) {
    HashKey* hash_key = (HashKey*)key;
    if (hash_key->type == TYPE_INT)
    {
        return dictGenHashFunction(&hash_key->data.i32, sizeof(hash_key->data.i32));
    }
    else if (hash_key->type == TYPE_INT64)
    {
        return dictGenHashFunction(&hash_key->data.i64, sizeof(hash_key->data.i64));
    }
    else if (hash_key->type == TYPE_CSTRING)
    {
        return dictGenHashFunction(hash_key->data.ptr, strlen((char*)hash_key->data.ptr));
    }
    else
    {
        return 0;
    }
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

void* HashTable::Get(HashKey* hash_key)
{
    dictEntry* de = dictFind(dict_, hash_key);
    if (de) return de->v.val;
    return NULL;
}
