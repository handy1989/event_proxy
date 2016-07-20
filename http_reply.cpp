#include "http_reply.h"
#include "store_entry.h"
#include "http_callbacks.h"
#include "logger.h"
#include "defines.h"

#include "event2/buffer.h"

#include <stdlib.h>
#include <sys/queue.h>
#include <event.h>

#include <vector>
#include <list>

using std::vector;
using std::list;

void ReplyClientHeader(StoreClient* client, StoreEntry* store_entry)
{
    struct evkeyvalq* headers = store_entry->mem_obj_->headers;
    struct evkeyvalq* client_header = evhttp_request_get_output_headers(client->request);
    struct evkeyval* header;
    LOG_DEBUG("reply client header, client:" << client);
    for (header = headers->tqh_first; header; header = header->next.tqe_next)
    {
        LOG_DEBUG("header, " << header->key << ":" << header->value);
        evhttp_add_header(client_header, header->key, header->value);
    }
    evhttp_add_header(client_header, "TestCache", client->hit == 1 ? "Hit" : "Miss");
    LOG_DEBUG("header, " << "TestCache:" << (client->hit == 1 ? "Hit" : "Miss"));
    client->reply_header_done = true;
    evhttp_send_reply_start(client->request, store_entry->code_, store_entry->code_str_.c_str());
}

void ReplyClientBody(StoreClient* client, StoreEntry* store_entry)
{
    vector<struct evbuffer*>& bodies = store_entry->mem_obj_->bodies;
    while (client->body_piece_index < bodies.size())
    {
        int len = evbuffer_get_length(bodies[client->body_piece_index]);
        char* buf = (char*)malloc(len);
        evbuffer_copyout(bodies[client->body_piece_index], buf, len);
        struct evbuffer* evbuf = evbuffer_new();
        evbuffer_add(evbuf, buf, len);
        evhttp_send_reply_chunk(client->request, evbuf);
        ++client->body_piece_index;

        free(buf);
        evbuffer_free(evbuf);

        client->reply_body_size += len;

        LOG_DEBUG("reply body, size:" << len << " client:" << client->request
                << " reply_size:" << client->reply_body_size << " body_size:" << store_entry->body_size_);
    }
}

void ReplyClient(RequestCtx* request_ctx)
{
    StoreEntry* store_entry = request_ctx->store_entry;
    MemObj* mem_obj = store_entry->mem_obj_;

    store_entry->Lock(); // 这个锁加的有点大，如果回复数据量很大，对其它请求会有影响
    LOG_INFO("lock entry:" << store_entry);
    for (list<StoreClient*>::iterator it = store_entry->store_clients_.begin(); it != store_entry->store_clients_.end();)
    {
        if (mem_obj->headers)
        {
            // 已完成包头接收，给客户端回包头
            if (!(*it)->reply_header_done)
            {
                ReplyClientHeader(*it, store_entry);
            }
        }
        if ((*it)->body_piece_index < mem_obj->bodies.size())
        {
            ReplyClientBody(*it, store_entry);
        }
        if ((*it)->body_piece_index >= mem_obj->body_piece_num - 1)
        {
            evhttp_send_reply_end((*it)->request);
            LOG_INFO("reply client finished, free client, request:" << (*it)->request);
            SAFELY_DELETE(*it);
            store_entry->store_clients_.erase(it++);

        }
        else
        {
            it++;
        }
        
    }
    if (store_entry->store_clients_.size() == 0)
    {
        if (request_ctx->comm_timer)
        {
            event_del(request_ctx->comm_timer);
            event_free(request_ctx->comm_timer);
            request_ctx->comm_timer = NULL;
            LOG_INFO("free comm_timer, request:" << request_ctx->client_request);
        }
    }
    LOG_INFO("unlock entry:" << store_entry);
    store_entry->Unlock();
}
