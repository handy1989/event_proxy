#ifndef _HTTP_CALLBACKS_H_
#define _HTTP_CALLBACKS_H_

#include "http_service.h"

struct CallbackPara
{
    HttpService* http_service;
    struct evhttp_request* client_request;
    struct evhttp_request* remote_request;
    struct evhttp_connection* client_conn;
    struct evhttp_conecction* remote_conn;
};

void HttpGenericCallback(struct evhttp_request* req, void* arg);

#endif // _HTTP_CALLBACKS_H_