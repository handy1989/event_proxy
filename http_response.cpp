#include "http_response.h"
#include "utility.h"
#include "logger.h"

#include <string.h>
#include <stdlib.h>

#include <string>

using std::string;

int32_t HttpResponse::ParseFirstLine(char* line)
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
            if (CreateHttpVersion(begin, end, http_version) != 0)
            {
                LOG_WARNING("create http_version failed, remain_str:" << begin);
                break;
            }

        }
        else if (count == 2)
            http_code_ = atoi(string(begin, end - begin).c_str());
        else if (count == 3)
            http_code_str_ = string(begin, end - begin);
        else
        {
            LOG_WARNING("format of first req line is error:" << line);
            break;
        }
        begin = end;
    }
    first_line_parsed_ = true;
    return count == 3 ? 0 : 1;
}

int32_t HttpResponse::ParseHeaderLine(char* line)
{
    return header.Parse(line);
}
