#include "http_header.h"

static const HttpHeaderFieldAttrs HeadersAttrs[] =
{
        {"Accept", HDR_ACCEPT, ftStr},

        {"Accept-Charset", HDR_ACCEPT_CHARSET, ftStr},
        {"Accept-Encoding", HDR_ACCEPT_ENCODING, ftStr},
        {"Accept-Language", HDR_ACCEPT_LANGUAGE, ftStr},
        {"Accept-Ranges", HDR_ACCEPT_RANGES, ftStr},
        {"Age", HDR_AGE, ftInt},
        {"Allow", HDR_ALLOW, ftStr},
        {"Authorization", HDR_AUTHORIZATION, ftStr},	/* for now */
        {"Cache-Control", HDR_CACHE_CONTROL, ftPCc},
        {"Connection", HDR_CONNECTION, ftStr},
        {"Content-Base", HDR_CONTENT_BASE, ftStr},
        {"Content-Disposition", HDR_CONTENT_DISPOSITION, ftStr},  /* for now */
        {"Content-Encoding", HDR_CONTENT_ENCODING, ftStr},
        {"Content-Language", HDR_CONTENT_LANGUAGE, ftStr},
        {"Content-Length", HDR_CONTENT_LENGTH, ftInt64},
        {"Content-Location", HDR_CONTENT_LOCATION, ftStr},
        {"Content-MD5", HDR_CONTENT_MD5, ftStr},	/* for now */
        {"Content-Range", HDR_CONTENT_RANGE, ftPContRange},
        {"Content-Type", HDR_CONTENT_TYPE, ftStr},
        {"Cookie", HDR_COOKIE, ftStr},
        {"Date", HDR_DATE, ftDate_1123},
        {"ETag", HDR_ETAG, ftETag},
        {"Expires", HDR_EXPIRES, ftDate_1123},
        {"From", HDR_FROM, ftStr},
        {"Host", HDR_HOST, ftStr},
        {"If-Match", HDR_IF_MATCH, ftStr},	/* for now */
        {"If-Modified-Since", HDR_IF_MODIFIED_SINCE, ftDate_1123},
        {"If-None-Match", HDR_IF_NONE_MATCH, ftStr},	/* for now */
        {"If-Range", HDR_IF_RANGE, ftDate_1123_or_ETag},
        {"Keep-Alive", HDR_KEEP_ALIVE, ftStr},
        {"Last-Modified", HDR_LAST_MODIFIED, ftDate_1123},
        {"Link", HDR_LINK, ftStr},
        {"Location", HDR_LOCATION, ftStr},
        {"Max-Forwards", HDR_MAX_FORWARDS, ftInt64},
        {"Mime-Version", HDR_MIME_VERSION, ftStr},	/* for now */
        {"Pragma", HDR_PRAGMA, ftStr},
        {"Proxy-Authenticate", HDR_PROXY_AUTHENTICATE, ftStr},
        {"Proxy-Authentication-Info", HDR_PROXY_AUTHENTICATION_INFO, ftStr},
        {"Proxy-Authorization", HDR_PROXY_AUTHORIZATION, ftStr},
        {"Proxy-Connection", HDR_PROXY_CONNECTION, ftStr},
        {"Public", HDR_PUBLIC, ftStr},
        {"Range", HDR_RANGE, ftPRange},
        {"Referer", HDR_REFERER, ftStr},
        {"Request-Range", HDR_REQUEST_RANGE, ftPRange},	/* usually matches HDR_RANGE */
        {"Retry-After", HDR_RETRY_AFTER, ftStr},	/* for now (ftDate_1123 or ftInt!) */
        {"Server", HDR_SERVER, ftStr},
        {"Set-Cookie", HDR_SET_COOKIE, ftStr},
        {"TE", HDR_TE, ftStr},
        {"Title", HDR_TITLE, ftStr},
        {"Trailers", HDR_TRAILERS, ftStr},
        {"Transfer-Encoding", HDR_TRANSFER_ENCODING, ftStr},
        {"Translate", HDR_TRANSLATE, ftStr},	/* for now. may need to crop */
        {"Unless-Modified-Since", HDR_UNLESS_MODIFIED_SINCE, ftStr},  /* for now ignore. may need to crop */
        {"Upgrade", HDR_UPGRADE, ftStr},	/* for now */
        {"User-Agent", HDR_USER_AGENT, ftStr},
        {"Vary", HDR_VARY, ftStr},	/* for now */
        {"Via", HDR_VIA, ftStr},	/* for now */
        {"Warning", HDR_WARNING, ftStr},	/* for now */
        {"WWW-Authenticate", HDR_WWW_AUTHENTICATE, ftStr},
        {"Authentication-Info", HDR_AUTHENTICATION_INFO, ftStr},
        {"X-Cache", HDR_X_CACHE, ftStr},
        {"X-Cache-Lookup", HDR_X_CACHE_LOOKUP, ftStr},
        {"X-Forwarded-For", HDR_X_FORWARDED_FOR, ftStr},
        {"X-Request-URI", HDR_X_REQUEST_URI, ftStr},
        {"X-Squid-Error", HDR_X_SQUID_ERROR, ftStr},
        {"Negotiate", HDR_NEGOTIATE, ftStr},
        {"Surrogate-Capability", HDR_SURROGATE_CAPABILITY, ftStr},
        {"Surrogate-Control", HDR_SURROGATE_CONTROL, ftPSc},
        {"Front-End-Https", HDR_FRONT_END_HTTPS, ftStr},
        {"Other:", HDR_OTHER, ftStr}
};

int32_t HttpHeader::Parse(char* line)
{
    return 0;
}

