#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "http_msg.h"
#include <stdint.h>

class HttpRequest : public HttpMsg
{
public:
    HttpRequest();
    int32_t ParseFirstLine(char* line);
    int32_t ParseHeaderLine(char* line);

    std::string url;
};

#endif // _HTTP_REQUEST_H_
