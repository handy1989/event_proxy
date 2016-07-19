#include "store_entry.h"

using std::string;
using std::list;

StoreEntry::StoreEntry(const string& url) : url_(url)
{
    completion_ = false;
}

void StoreEntry::Lock()
{
    //lock_.lock();
}

void StoreEntry::Unlock()
{
    //lock_.unlock();
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
