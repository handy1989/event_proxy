#ifndef _SERVER_SIDE_CALLBACKS_H_
#define _SERVER_SIDE_CALLBACKS_H_

#include "global.h"
#include "event_includes.h"

void read_remote_cb(struct bufferevent* bev, void* arg);
void write_remote_cb(struct bufferevent* bev, void* arg);
void event_remote_cb(struct bufferevent* bev, short events, void* arg);

void ReplyHeader(BufferContext* buffer_context);

#endif // _SERVER_SIDE_CALLBACKS_H_
