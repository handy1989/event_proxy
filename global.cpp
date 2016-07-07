#include "global.h"

#include <assert.h>
#include <map>

using std::map;

map<bufferevent*, BufferContext*> client_context;
map<bufferevent*, BufferContext*> remote_context;

void AddClientBufferContext(bufferevent* bev, BufferContext* buffer_context)
{
    assert(buffer_context);
    assert(bev == buffer_context->client);

    client_context[bev] = buffer_context;
}

BufferContext* GetClientBufferContext(bufferevent* bev)
{
    std::map<bufferevent*, BufferContext*>::iterator it = client_context.find(bev);
    if (it == client_context.end())
    {
        return NULL;
    }
    return it->second;
}

void AddRemoteBufferContext(bufferevent* bev, BufferContext* buffer_context)
{
    assert(buffer_context);
    assert(bev == buffer_context->remote);

    remote_context[bev] = buffer_context;
}

BufferContext* GetRemoteBufferContext(bufferevent* bev)
{
    std::map<bufferevent*, BufferContext*>::iterator it = remote_context.find(bev);
    if (it == remote_context.end())
    {
        return NULL;
    }
    return it->second;
}
