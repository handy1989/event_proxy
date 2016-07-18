#ifndef _HTTP_CALLBACKS_H_
#define _HTTP_CALLBACKS_H_

#include "http_service.h"

struct CallbackPara
{
    HttpService* http_service;
    struct evhttp_uri* uri;
    struct evhttp_request* client_request;
    struct evhttp_request* remote_request;
    struct evhttp_connection* client_conn;
    struct evhttp_connection* remote_conn;

    struct event* clean_timer;
};

void ConnectRemote(CallbackPara* callback_para);

void HttpGenericCallback(struct evhttp_request* req, void* arg);
void ReplyCompleteCallback(struct evhttp_request* request, void* arg);
void ClientConnectionCloseCallback(struct evhttp_connection* connection, void* arg);
void RemoteReadCallback(struct evhttp_request* response, void* arg);
int ReadHeaderDoneCallback(struct evhttp_request* request, void* arg);
void ReadChunkCallback(struct evhttp_request* remote_response, void* arg);
void RemoteRequestErrorCallback(enum evhttp_request_error error, void* arg);
void SendRemoteCompleteCallback(struct evhttp_request* request, void* arg);
void RemoteConnectionCloseCallback(struct evhttp_connection* connection, void* arg);
void FreeRemoteConnCallback(int sock, short which, void* arg);

#endif // _HTTP_CALLBACKS_H_
