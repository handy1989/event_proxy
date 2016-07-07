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
        ++count;
        if (count == 1)
        {
            method = CreateHttpMethod(begin, end);
            if (method == METHOD_NONE)
            {
                LOG_WARNING("method unkown, remain_str:" << begin);
                break;
            }
        }
        else if (count == 2)
        {
            if (*begin == '/') 
                url_path = string(begin, end - begin);
            else if (strncmp(begin, "http://", 7) == 0)
            {
                for (char* p = begin + 7; p < end; ++p)
                {
                    if (*p == '/')
                    {
                        url_path = string(p, end - p);
                        break;
                    }
                }
            }
            else
            {
                url_path = "";
            }

            url = string(begin, end - begin);
        }
        else if (count == 3)
        {
            if (CreateHttpVersion(begin, end, http_version) != 0)
            {
                LOG_WARNING("create http_version failed, remain_str:" << begin);
                break;
            }
        }
        else
        {
            LOG_WARNING("format of first req line is error:" << line);
            break;
        }
        begin = end;
    }
    return count == 3 ? 0 : 1;
}

int32_t HttpRequest::ParseHeaderLine(char* line)
{
    return header.Parse(line);
}
