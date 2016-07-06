#ifndef _HTTP_MSG_H_
#define _HTTP_MSG_H_

#include "http_header.h"

#include <stdint.h>

enum method_t {
    METHOD_NONE,		/* 000 */
    METHOD_GET,			/* 001 */
    METHOD_POST
};

class HttpMethod
{
public:
    HttpMethod(const char* method);
    method_t method;
};

class HttpMsg
{
public:
    std::string url_path;
    method_t method;
    uint64_t content_length;

    std::string ip;
    uint16_t port;

    uint32_t fd;

    HttpHeader header;
};

#endif // _HTTP_MSG_H_
