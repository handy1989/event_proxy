#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <string>

#include "http_request.h"
#include "http_response.h"
#include "event2/bufferevent.h"

struct BufferContext
{
    BufferContext() : 
        first_line_parsed(false), 
        read_header_finished(false),
        write_remote_finished(false) {}
    bufferevent* client;
    bufferevent* remote;
    bool first_line_parsed;
    bool read_header_finished;
    HttpRequest* http_request;

    int32_t response_status;
    std::string remote_host;
    std::string remote_ip;
    int32_t remote_port;

    HttpResponse http_response;
    bool write_remote_finished;
};

void AddClientBufferContext(bufferevent* bev, BufferContext* buffer_context);
BufferContext* GetClientBufferContext(bufferevent* bev);

void AddRemoteBufferContext(bufferevent* bev, BufferContext* buffer_context);
BufferContext* GetRemoteBufferContext(bufferevent* bev);

#endif // _GLOBAL_H_
