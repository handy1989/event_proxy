#ifndef STORE_ENTRY_H_
#define STORE_ENTRY_H_

#include "lock.h"

#include "event2/buffer.h"

#include <sys/queue.h>
#include <event.h>

#include <string>
#include <list>
#include <vector>

struct StoreClient
{
    StoreClient()
    {
        body_piece_index = 0;
        reply_header_done = false;
        reply_body_size = 0;
        hit = 0;
    }

    struct evhttp_request* request; 
    unsigned int body_piece_index;
    bool reply_header_done;
    int hit;
    int reply_body_size;
};

struct MemObj
{
    MemObj() 
    {
        headers = NULL;
        body_piece_num = -1;
    }

    struct evkeyvalq* headers;
    std::vector<struct evbuffer*> bodies;
    unsigned int body_piece_num;
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
    std::list<StoreClient*> store_clients_;

    bool completion_;
    int body_size_;

private:
    std::string url_;

    SafeLock lock_;
};

#endif // STORE_ENTRY_H_
