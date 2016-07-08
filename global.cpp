#include "global.h"
#include "logger.h"

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
    LOG_DEBUG("add client bev:" << bev << " buffer_context:" << buffer_context);
}

BufferContext* GetClientBufferContext(bufferevent* bev)
{
    map<bufferevent*, BufferContext*>::iterator it = client_context.find(bev);
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
    LOG_DEBUG("add remote bev:" << bev << " buffer_context:" << buffer_context);
}

BufferContext* GetRemoteBufferContext(bufferevent* bev)
{
    map<bufferevent*, BufferContext*>::iterator it = remote_context.find(bev);
    if (it == remote_context.end())
    {
        return NULL;
    }
    return it->second;
}

void RemoveBufferContext(BufferContext* buffer_context)
{
    assert(buffer_context);
    bufferevent* client = buffer_context->client;
    bufferevent* remote = buffer_context->remote;
    map<bufferevent*, BufferContext*>::iterator it_client = client_context.find(client);
    if (it_client != client_context.end())
    {
        client_context.erase(it_client);
    }
    map<bufferevent*, BufferContext*>::iterator it_remote = remote_context.find(remote);
    if (it_remote != remote_context.end())
    {
        remote_context.erase(it_remote);
    }
    LOG_DEBUG("remove buffer_context:" << buffer_context
            << " client:" << buffer_context->client
            << " remote:" << buffer_context->remote);
    SAFE_DELETE(buffer_context->http_request);
    SAFE_DELETE(buffer_context->http_response);
    SAFE_DELETE(buffer_context);
}
