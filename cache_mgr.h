#ifndef CACHE_MGR_H_
#define CACHE_MGR_H_

#include "lock.h"

#include <map>
#include <string>

class StoreEntry;

class CacheMgr
{
public:
    CacheMgr() {}
    ~CacheMgr() {}
    StoreEntry* GetStoreEntry(const std::string& url);

private:
    StoreEntry* CreateStoreEntry(const std::string& url);

private:
    std::map<std::string, StoreEntry*> cache_items_;

    SafeLock lock_;
};

#endif // CACHE_MGR_H_
