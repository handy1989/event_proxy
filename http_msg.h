#ifndef _HTTP_MSG_H_
#define _HTTP_MSG_H_

#include "http_header.h"

#include <stdint.h>
#include <iostream>

enum method_t
{
    METHOD_NONE,		/* 000 */
    METHOD_GET,			/* 001 */
    METHOD_POST
};


struct HttpVersion
{
    int32_t major;
    int32_t minor;
};

std::ostream& operator << (std::ostream& stream, HttpVersion& http_version);

method_t CreateHttpMethod(char* begin, char* end);
int32_t CreateHttpVersion(char* begin, char* end, HttpVersion& http_version);

class HttpMsg
{
public:
    std::string url_path;
    method_t method;
    HttpVersion http_version;
    uint64_t content_length;

    std::string ip;
    uint16_t port;

    uint32_t fd;

    HttpHeader header;
};

#endif // _HTTP_MSG_H_
