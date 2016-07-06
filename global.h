#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "http_request.h"
#include "event2/bufferevent.h"

struct BufferContext
{
    BufferContext() : 
        first_line_parsed(false), 
        read_header_finished(false) {}
    bufferevent* client;
    bufferevent* remote;
    bool first_line_parsed;
    bool read_header_finished;
    HttpRequest http_request;
};

void AddClientBufferContext(bufferevent* bev, BufferContext* buffer_context);
BufferContext* GetClientBufferContext(bufferevent* bev);

#endif // _GLOBAL_H_
