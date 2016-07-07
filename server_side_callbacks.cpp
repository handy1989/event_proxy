#include "server_side_callbacks.h"
#include "logger.h"
#include "http_parse_help.h"
#include "service.h"
#include "global.h"
#include "event_includes.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <vector>

using std::vector;

void read_remote_cb(struct bufferevent* bev, void* arg)
{
    //char buf[1024];
    //int n;
    //struct evbuffer *input = bufferevent_get_input(bev);
    //while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
    //    fwrite(buf, 1, n, stdout);
    //}  
    struct evbuffer* input = bufferevent_get_input(bev);

    BufferContext* buffer_context = GetRemoteBufferContext(bev);
    assert(buffer_context);
    assert(buffer_context->remote == bev);

    size_t line_len;
    while (true)
    {
        char* line = evbuffer_readln(input, &line_len, EVBUFFER_EOL_CRLF_STRICT);
        if (!line)
        {
            LOG_DEBUG("break");
            break;
        }
        if (!buffer_context->http_response.first_line_parsed_)
        {
            LOG_DEBUG("parse first line:" << line);
            int32_t ret = buffer_context->http_response.ParseFirstLine(line);

            LOG_DEBUG("parse first response line, http_version:" << buffer_context->http_response.http_version
                << " http_code:" << buffer_context->http_response.http_code_
                << " http_code_str:" << buffer_context->http_response.http_code_str_
                << " ret:" << ret);
        }
        else if (strlen(line) == 0)
        {
            buffer_context->http_response.read_header_finished_ = true;
            LOG_DEBUG("read header finished, content_length:" << buffer_context->http_response.header.content_length_);
        }
        else
        {
            buffer_context->http_request.ParseHeaderLine(line);
            LOG_DEBUG("parse line:" << line);
        }
        free(line);
    }
}

void write_remote_cb(struct bufferevent* bev, void* arg)
{
    BufferContext* buffer_context = GetRemoteBufferContext(bev);
    assert(buffer_context);
    assert(buffer_context->remote == bev);
    
    if (!buffer_context->write_remote_finished)
    {
        LOG_DEBUG("send request to remote server");

        struct evbuffer *output = bufferevent_get_output(bev);
        evbuffer_add_printf(output, "%s %s HTTP/%d.%d\r\n", MethodStr(buffer_context->http_request.method), 
                buffer_context->http_request.url_path.c_str(),
                buffer_context->http_request.http_version.major, buffer_context->http_request.http_version.minor);
        for (vector<HttpHeaderEntry>::iterator it = buffer_context->http_request.header.entries.begin();
                it != buffer_context->http_request.header.entries.end(); ++it)
        {
            evbuffer_add_printf(output, "%s: %s\r\n", it->name.c_str(), it->value.c_str());
        }
        evbuffer_add_printf(output, "\r\n");

        buffer_context->write_remote_finished = true;
        bufferevent_enable(bev, EV_READ);

        LOG_DEBUG("write_remote_cb finished");
        
    }
}

void event_remote_cb(struct bufferevent* bev, short events, void* arg)
{
    BufferContext* buffer_context = GetRemoteBufferContext(bev);
    assert(buffer_context);
    assert(buffer_context->remote == bev);
    if (events & BEV_EVENT_CONNECTED) 
    {
        LOG_INFO("Connect " << buffer_context->remote_host 
                 << ":" << buffer_context->remote_port << " okay");
        //bufferevent_enable(bev, EV_WRITE);
        write_remote_cb(bev, NULL);
    } 
    else if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) 
    {
        if (events & BEV_EVENT_ERROR) 
        {
            int err = bufferevent_socket_get_dns_error(bev);
            if (err)
            {
                LOG_ERROR("DNS error:" << evutil_gai_strerror(err));
                buffer_context->response_status = 400;
                bufferevent_enable(buffer_context->client, EV_WRITE);
            }
        }
        bufferevent_free(bev);
    }
}
