#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <string>

#include "http_request.h"
#include "http_response.h"
#include "event2/bufferevent.h"

#define SAFE_DELETE(p) do {if(p) delete p; p = NULL;} while(0);
#define SAFE_FREE(p) do {if(p) free(p); p = NULL;} while(0);

struct BufferContext
{
    BufferContext() : 
        first_line_parsed(false), 
        read_header_finished(false),
        write_remote_finished(false),
        read_remote_body_finished(false) {}

    bufferevent* client;
    bufferevent* remote;

    // client_side
    bool first_line_parsed;
    bool read_header_finished;
    HttpRequest* http_request;

    int32_t response_status;
    std::string remote_host;
    std::string remote_ip;
    int32_t remote_port;

    // remote_side
    HttpResponse* http_response;
    bool write_remote_finished;
    bool read_remote_body_finished;
};

void AddClientBufferContext(bufferevent* bev, BufferContext* buffer_context);
BufferContext* GetClientBufferContext(bufferevent* bev);

void AddRemoteBufferContext(bufferevent* bev, BufferContext* buffer_context);
BufferContext* GetRemoteBufferContext(bufferevent* bev);

void RemoveBufferContext(BufferContext* buffer_context);

#endif // _GLOBAL_H_
