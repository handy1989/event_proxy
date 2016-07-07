#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include "http_msg.h"
#include <stdint.h>
#include <string>

class HttpResponse : public HttpMsg
{
public:
    HttpResponse() : first_line_parsed_(false),
        read_header_finished_(false) {}
    int32_t ParseFirstLine(char* line);
    int32_t ParseHeaderLine(char* line);

    uint32_t http_code_;
    std::string http_code_str_;

    bool first_line_parsed_;
    bool read_header_finished_;
};

#endif // _HTTP_RESPONSE_H_
