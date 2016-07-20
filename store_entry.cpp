#include "store_entry.h"

using std::string;
using std::list;

StoreEntry::StoreEntry(const string& url) : url_(url)
{
    completion_ = false;
    body_size_ = -1;
    status_ = STORE_INIT;
}

void StoreEntry::Lock(const ELockType type)
{
    if (type == READ_LOCKER)
    {
        lock_.rdlock();
    }
    else
    {
        lock_.wrlock();
    }
}

void StoreEntry::Unlock()
{
    lock_.unlock();
}

void StoreEntry::AddClient(StoreClient* client)
{
    store_clients_.push_back(client);
}

int StoreEntry::GetClientNum() 
{ 
    return store_clients_.size();
}

void StoreEntry::DelClient(StoreClient* client)
{
    for (list<StoreClient*>::iterator it = store_clients_.begin(); it != store_clients_.end(); ++it)
    {
        if (*it == client)
        {
            store_clients_.erase(it);
            break;
        }
    }
}

string StoreEntry::StatusStr()
{
    switch (status_)
    {
        case STORE_INIT:
            return "STORE_INIT";
            break;
        case STORE_PENDING:
            return "STORE_PENDING";
            break;
        case STORE_FINISHED:
            return "STORE_FINISHED";
            break;
        case STORE_ERROR:
            return "STORE_ERROR";
            break;
        default:
            return "STORE_UNKOWN";
            break;
    }
}
