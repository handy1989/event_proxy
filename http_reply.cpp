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
    client->reply_header_done = true;
    evhttp_send_reply_start(client->request, 200, "OK");
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

        LOG_DEBUG("reply body, size:" << len << " client:" << client 
                << " reply_size:" << client->reply_body_size << " body_size:" << store_entry->body_size_);
    }
}

void ReplyClient(RequestCtx* request_ctx)
{
    StoreEntry* store_entry = request_ctx->store_entry;
    MemObj* mem_obj = store_entry->mem_obj_;

    store_entry->Lock(); // 这个锁加的有点大，如果回复数据量很大，对其它请求会有影响
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
            LOG_INFO("reply client finished, free client:" << *it);
            SAFELY_DELETE(*it);
            store_entry->store_clients_.erase(it++);
        }
        else
        {
            it++;
        }
        
    }
    store_entry->Unlock();
}
