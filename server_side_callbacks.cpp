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

void ReadRemoteHeader(BufferContext* buffer_context)
{
//    {
//        char buf[1024];
//        int n;
//        struct evbuffer *input = bufferevent_get_input(buffer_context->remote);
//        while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
//            fwrite(buf, 1, n, stdout);
//        }
//        return ;
//
//    }
//
    struct evbuffer* input = bufferevent_get_input(buffer_context->remote);
    HttpResponse* http_response = buffer_context->http_response;
    char* line = NULL;
    while (true)
    {
        line = evbuffer_readln(input, NULL, EVBUFFER_EOL_CRLF_STRICT);
        LOG_DEBUG("read remote line:" << line);
        if (!line)
        {
            LOG_DEBUG("break");
            break;
        }
        if (!http_response->first_line_parsed_)
        {
            LOG_DEBUG("parse first line:" << line);
            int32_t ret = http_response->ParseFirstLine(line);

            LOG_DEBUG("parse first response line, http_version:" << http_response->http_version
                << " http_code:" << http_response->http_code_
                << " http_code_str:" << http_response->http_code_str_
                << " ret:" << ret);
        }
        else if (strlen(line) == 0)
        {
            http_response->read_header_finished_ = true;
            http_response->remain_body_size_ = http_response->header.content_length_;
            ReplyHeader(buffer_context);
            LOG_DEBUG("read header finished, content_length:" << http_response->header.content_length_);
            break;
        }
        else
        {
            http_response->ParseHeaderLine(line);
            LOG_DEBUG("parse line:" << line);
        }
        SAFE_FREE(line);
    }
    SAFE_FREE(line);
}

void ReadRemoteBody(BufferContext* buffer_context)
{
    struct evbuffer* remote_input = bufferevent_get_input(buffer_context->remote);
    struct evbuffer* client_output = bufferevent_get_output(buffer_context->client);
    HttpResponse* http_response = buffer_context->http_response;

    LOG_DEBUG("read remote body, remain_size:" << http_response->remain_body_size_
            << " read_remote_body_finished:" << buffer_context->read_remote_body_finished
            << " buffer_context:" << buffer_context);
    while (int n = evbuffer_remove_buffer(remote_input, client_output, http_response->remain_body_size_))
    {
        http_response->remain_body_size_ -= n;
        LOG_DEBUG("remove " << n << " bytes, remain_body_size:" << http_response->remain_body_size_); 
    }
    if (http_response->remain_body_size_ == 0)
    {
        buffer_context->read_remote_body_finished = true;
        bufferevent_free(buffer_context->remote);
        LOG_INFO("read remote body finished, free remote:" << buffer_context->remote);
    }
    LOG_DEBUG("read remote body, remain_size:" << http_response->remain_body_size_
            << " read_remote_body_finished:" << buffer_context->read_remote_body_finished
            << " buffer_context:" << buffer_context);
}

void ReplyHeader(BufferContext* buffer_context)
{
    LOG_DEBUG("reply to bev:" << buffer_context->client);
    struct evbuffer* output = bufferevent_get_output(buffer_context->client);
    HttpResponse* http_response = buffer_context->http_response;
    evbuffer_add_printf(output, "HTTP/%d.%d %d %s\r\n", 
            http_response->http_version.major,
            http_response->http_version.minor,
            http_response->http_code_,
            http_response->http_code_str_.c_str());
    for (vector<HttpHeaderEntry>::iterator it = http_response->header.entries.begin();
            it != http_response->header.entries.end(); ++it)
    {
        evbuffer_add_printf(output, "%s: %s\r\n", it->name.c_str(), it->value.c_str());
        LOG_DEBUG(it->name << ": " << it->value);
    }
    evbuffer_add_printf(output, "\r\n");
}

void read_remote_cb(struct bufferevent* bev, void* arg)
{

    LOG_DEBUG("call read_remote_cb");
    //char* line = evbuffer_readln(bufferevent_get_input(bev), NULL, EVBUFFER_EOL_CRLF_STRICT);
    //if (line)
    //    LOG_DEBUG("read remote line:" << line);
    //return ;

    BufferContext* buffer_context = GetRemoteBufferContext(bev);
    if (!buffer_context)
    {
        bufferevent_disable(bev, EV_READ);
        return ;
    }

    assert(buffer_context->remote == bev);

    if (!buffer_context->http_response->read_header_finished_)
    {
        ReadRemoteHeader(buffer_context);
    }
    if (buffer_context->http_response->read_header_finished_)
    {
        ReadRemoteBody(buffer_context);
        if (buffer_context->read_remote_body_finished)
        {
            LOG_DEBUG("disable remote read");
            bufferevent_disable(bev, EV_READ);
        }
    }

}

void SendHeader(struct bufferevent* bev)
{
    BufferContext* buffer_context = GetRemoteBufferContext(bev);
    if (!buffer_context)
    {
        bufferevent_disable(bev, EV_WRITE);
        return ;
    }

    assert(buffer_context->remote == bev);
    
    LOG_DEBUG("send request to remote server");

    struct evbuffer *output = bufferevent_get_output(bev);
    evbuffer_add_printf(output, "%s %s HTTP/%d.%d\r\n", MethodStr(buffer_context->http_request->method), 
            buffer_context->http_request->url_path.c_str(),
            buffer_context->http_request->http_version.major, buffer_context->http_request->http_version.minor);
    for (vector<HttpHeaderEntry>::iterator it = buffer_context->http_request->header.entries.begin();
            it != buffer_context->http_request->header.entries.end(); ++it)
    {
        evbuffer_add_printf(output, "%s: %s\r\n", it->name.c_str(), it->value.c_str());
    }
    evbuffer_add_printf(output, "\r\n");

    buffer_context->write_remote_finished = true;
    bufferevent_enable(bev, EV_READ);

    LOG_DEBUG("SendHeader finished");
        
}

void event_remote_cb(struct bufferevent* bev, short events, void* arg)
{
    if (events & BEV_EVENT_CONNECTED) 
    {
        LOG_INFO("connect okay");
        //bufferevent_enable(bev, EV_WRITE);
        SendHeader(bev);
    } 
    else if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) 
    {
        if (events & BEV_EVENT_ERROR) 
        {
            int err = bufferevent_socket_get_dns_error(bev);
            if (err)
            {
                LOG_ERROR("DNS error:" << evutil_gai_strerror(err));
                BufferContext* buffer_context = GetRemoteBufferContext(bev);
                if (buffer_context)
                {
                    buffer_context->response_status = 400;
                    bufferevent_enable(buffer_context->client, EV_WRITE);
                }
            }
        }
        bufferevent_free(bev);
        LOG_DEBUG("free remote:" << bev);
    }
}
