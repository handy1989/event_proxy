#ifndef STORE_ENTRY_H_
#define STORE_ENTRY_H_

#include "lock.h"

#include <string>
#include <list>


struct StoreClient
{
   struct evhttp_request* client_request; 
   int offset;
};

class MemObj
{
public:
    MemObj() : object_size_(0), data_(NULL) {}
    int object_size_;
    void* data_;
};

class StoreEntry
{
public:
    StoreEntry(const std::string& url);
    ~StoreEntry();

    void Lock();
    void Unlock();

    void AddClient(StoreClient*);
    void DelClient(StoreClient*);
    int GetClientNum();

    MemObj* mem_obj_;

private:
    std::string url_;
    std::list<StoreClient*> store_clients_;

    SafeLock lock_;
};

#endif // STORE_ENTRY_H_
