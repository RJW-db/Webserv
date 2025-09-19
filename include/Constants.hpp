#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

constexpr size_t DISCONNECT_DELAY_SECONDS = 2;
constexpr char CRLF[] = "\r\n";
constexpr size_t CRLF_LEN = sizeof(CRLF) - 1;
constexpr char CRLF2[] = "\r\n\r\n";
constexpr size_t CRLF2_LEN = sizeof(CRLF2) - 1;
constexpr size_t BOUNDARY_PREFIX_LEN = 2;  // For "--" boundary prefix
constexpr size_t PIPE_BUFFER_SIZE = 512 * 1024; // 512 KB
constexpr size_t KILOBYTE = 1024;

typedef enum
{
    INFO_LOG                        = 0,
    OK                              = 200,
    CREATED                         = 201,
    MOVED_PERMANENTLY               = 301,
    FOUND                           = 302,
    SEE_OTHER                       = 303,
    TEMPORARY_REDIRECT              = 307,
    PERMANENT_REDIRECT              = 308,
    BAD_REQUEST                     = 400,
    FORBIDDEN                       = 403,
    NOT_FOUND                       = 404,
    METHOD_NOT_ALLOWED              = 405,
    PAYLOAD_TOO_LARGE               = 413,
    URI_TOO_LONG                    = 414,
    UNSUPPORTED_MEDIA_TYPE          = 415,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    INTERNAL_SERVER_ERROR           = 500,
    NOT_IMPLEMENTED                 = 501,
    BAD_GATEWAY                     = 502,
    SERVICE_UNAVAILABLE             = 503,
    GATEWAY_TIMEOUT                 = 504
} HttpStatusCode;
#endif
