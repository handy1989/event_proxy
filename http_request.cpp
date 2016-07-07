#include "http_request.h"
#include "utility.h"
#include "logger.h"

#include <string.h>

#include <string>

using std::string;

HttpRequest::HttpRequest()
{

}

int32_t HttpRequest::ParseFirstLine(char* line)
{
    char* begin = line;
    int32_t count = 0;
    while (true)
    {
        begin = FindFirstNotOf(begin, " \r\n");
        if (!begin) break;
        char* end = FindFirstOf(begin, " \r\n");
        if (!end)
        {
            end = begin + strlen(begin);
        }
        if (count == 0)
        {
            method = CreateHttpMethod(begin, end);
            if (method == METHOD_NONE)
            {
                LOG_WARNING("method unkown, remain_str:" << begin);
                return 1;
            }
        }
        else if (count == 1)
        {
            url = string(begin, end - begin);
        }
        else if (count == 2)
        {
            if (CreateHttpVersion(begin, end, http_version) != 0)
            {
                LOG_WARNING("create http_version failed, remain_str:" << begin);
                return 1;
            }
        }
        else
        {
            LOG_WARNING("format of first req line is error:" << line);
            return 1;
        }
        begin = end;
        ++count;
    }
    if (count != 3)
    {
        LOG_WARNING("format of first req line is error:" << line);
        return 1;
    }
    return 0;
}

int32_t HttpRequest::ParseHeaderLine(char* line)
{
    return header.Parse(line);
}