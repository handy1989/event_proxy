#include "cache_mgr.h"
#include "store_entry.h"

using std::string;
using std::map;

StoreEntry* CacheMgr::GetStoreEntry(const string& url)
{
    StoreEntry* entry = NULL;
    do
    {
        ScopedSafeLock lock(lock_);
        map<string, StoreEntry*>::iterator it = cache_items_.find(url);
        if (it != cache_items_.end())
        {
            entry = it->second;
            break;
        }

        // 没找到，创建空StoreEntry
        entry = CreateStoreEntry(url);
        cache_items_[url] = entry;
    } while(0);

    return entry;
}

StoreEntry* CacheMgr::CreateStoreEntry(const string& url)
{
    StoreEntry* entry = new StoreEntry(url);
    entry->mem_obj_ = new MemObj();

    return entry;
}
