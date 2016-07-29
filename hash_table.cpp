#include "hash_table.h"
#include "hash/dict.h"

#include <string.h>

dictType HashDictType = {
    DictEncObjHash,             /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictEncObjKeyCompare,       /* key compare */
    NULL, //dictRedisObjectDestructor,  /* key destructor */
    NULL //dictRedisObjectDestructor   /* val destructor */
};

unsigned int DictEncObjHash(const void *key) {
    HashKey* hash_key = (HashKey*)key;
    if (hash_key->type == TYPE_INT)
    {
        return dictGenHashFunction((unsigned char*)hash_key->data.i32, sizeof(hash_key->data.i32);
    }
    else if (hash_key->type == TYPE_INT64)
    {
        return dictGenHashFunction((unsigned char*)hash_key->data.i64, sizeof(hash_key->data.i64));
    }
    else if (hash_key->type == TYPE_CSTRING)
    {
        return dictGenHashFunction(hash_key->data.ptr, strlen((char*)hash_key->data.ptr));
    }
}

HashTable::HashTable()
{
    dict_ = dictCreate(&HashDictType, NULL);
}

HashTable::~HashTable()
{

}
