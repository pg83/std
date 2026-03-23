#include "reason.h"

using namespace stl;

StringView stl::reasonPhrase(u32 code) {
    switch (code) {
        case 200:
            return StringView(u8"OK");
        case 201:
            return StringView(u8"Created");
        case 204:
            return StringView(u8"No Content");
        case 301:
            return StringView(u8"Moved Permanently");
        case 302:
            return StringView(u8"Found");
        case 304:
            return StringView(u8"Not Modified");
        case 400:
            return StringView(u8"Bad Request");
        case 401:
            return StringView(u8"Unauthorized");
        case 403:
            return StringView(u8"Forbidden");
        case 404:
            return StringView(u8"Not Found");
        case 405:
            return StringView(u8"Method Not Allowed");
        case 500:
            return StringView(u8"Internal Server Error");
        case 502:
            return StringView(u8"Bad Gateway");
        case 503:
            return StringView(u8"Service Unavailable");
    }

    return StringView(u8"Unknown");
}
