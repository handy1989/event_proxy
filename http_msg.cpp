#include "http_msg.h"

#include <string.h>
#include <iostream>

const char *RequestMethodStr[] =
{
    "NONE",
    "GET",
    "POST",
    NULL
};

std::ostream& operator << (std::ostream& stream, HttpVersion& http_version)
{
    return stream << http_version.major << "." << http_version.minor;
}

method_t CreateHttpMethod(char* begin, char* end)
{
    method_t method = METHOD_NONE;
    for (int32_t i = 0; RequestMethodStr[i] != NULL; ++i)
    {
        if (strncmp(RequestMethodStr[i], begin, end - begin) == 0)
        {
            method = (method_t)i;
            break;
        }
    }
    return method;
}

int32_t CreateHttpVersion(char* begin, char* end, HttpVersion& http_version)
{
    if (strncmp(begin, "HTTP/", 5) != 0)
    {
        return 1;
    }
    if (end - begin != 8)
    {// HTTP/1.0 length is 8
        return 1;
    }
    char* p = strchr(begin, '.');
    if (!p)
    {
        return 1;
    }
    http_version.major = *(begin + 5) - '0';
    http_version.minor = *(p + 1) - '0';
    if (http_version.major > 9 || http_version.major < 0 
            || http_version.minor > 9 || http_version.minor < 0)
    {
        return 1;
    }
    return 0;
}
