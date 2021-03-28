#ifndef TIHI_HTTP_HTTP_H_
#define TIHI_HTTP_HTTP_H_

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace tihi {

namespace http {

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                   \
    XX(100, CONTINUE, Continue)                                               \
    XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                         \
    XX(102, PROCESSING, Processing)                                           \
    XX(200, OK, OK)                                                           \
    XX(201, CREATED, Created)                                                 \
    XX(202, ACCEPTED, Accepted)                                               \
    XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)   \
    XX(204, NO_CONTENT, No Content)                                           \
    XX(205, RESET_CONTENT, Reset Content)                                     \
    XX(206, PARTIAL_CONTENT, Partial Content)                                 \
    XX(207, MULTI_STATUS, Multi - Status)                                     \
    XX(208, ALREADY_REPORTED, Already Reported)                               \
    XX(226, IM_USED, IM Used)                                                 \
    XX(300, MULTIPLE_CHOICES, Multiple Choices)                               \
    XX(301, MOVED_PERMANENTLY, Moved Permanently)                             \
    XX(302, FOUND, Found)                                                     \
    XX(303, SEE_OTHER, See Other)                                             \
    XX(304, NOT_MODIFIED, Not Modified)                                       \
    XX(305, USE_PROXY, Use Proxy)                                             \
    XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                           \
    XX(308, PERMANENT_REDIRECT, Permanent Redirect)                           \
    XX(400, BAD_REQUEST, Bad Request)                                         \
    XX(401, UNAUTHORIZED, Unauthorized)                                       \
    XX(402, PAYMENT_REQUIRED, Payment Required)                               \
    XX(403, FORBIDDEN, Forbidden)                                             \
    XX(404, NOT_FOUND, Not Found)                                             \
    XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                           \
    XX(406, NOT_ACCEPTABLE, Not Acceptable)                                   \
    XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)     \
    XX(408, REQUEST_TIMEOUT, Request Timeout)                                 \
    XX(409, CONFLICT, Conflict)                                               \
    XX(410, GONE, Gone)                                                       \
    XX(411, LENGTH_REQUIRED, Length Required)                                 \
    XX(412, PRECONDITION_FAILED, Precondition Failed)                         \
    XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                             \
    XX(414, URI_TOO_LONG, URI Too Long)                                       \
    XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                   \
    XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                     \
    XX(417, EXPECTATION_FAILED, Expectation Failed)                           \
    XX(421, MISDIRECTED_REQUEST, Misdirected Request)                         \
    XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                       \
    XX(423, LOCKED, Locked)                                                   \
    XX(424, FAILED_DEPENDENCY, Failed Dependency)                             \
    XX(426, UPGRADE_REQUIRED, Upgrade Required)                               \
    XX(428, PRECONDITION_REQUIRED, Precondition Required)                     \
    XX(429, TOO_MANY_REQUESTS, Too Many Requests)                             \
    XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
    XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)     \
    XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                     \
    XX(501, NOT_IMPLEMENTED, Not Implemented)                                 \
    XX(502, BAD_GATEWAY, Bad Gateway)                                         \
    XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                         \
    XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                 \
    XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)           \
    XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                 \
    XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                       \
    XX(508, LOOP_DETECTED, Loop Detected)                                     \
    XX(510, NOT_EXTENDED, Not Extended)                                       \
    XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

enum http_status {
#define XX(num, name, string) HTTP_STATUS_##name = num,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/* Request Methods */
#define HTTP_METHOD_MAP(XX)          \
    XX(0, DELETE, DELETE)            \
    XX(1, GET, GET)                  \
    XX(2, HEAD, HEAD)                \
    XX(3, POST, POST)                \
    XX(4, PUT, PUT)                  \
    /* pathological */               \
    XX(5, CONNECT, CONNECT)          \
    XX(6, OPTIONS, OPTIONS)          \
    XX(7, TRACE, TRACE)              \
    /* WebDAV */                     \
    XX(8, COPY, COPY)                \
    XX(9, LOCK, LOCK)                \
    XX(10, MKCOL, MKCOL)             \
    XX(11, MOVE, MOVE)               \
    XX(12, PROPFIND, PROPFIND)       \
    XX(13, PROPPATCH, PROPPATCH)     \
    XX(14, SEARCH, SEARCH)           \
    XX(15, UNLOCK, UNLOCK)           \
    XX(16, BIND, BIND)               \
    XX(17, REBIND, REBIND)           \
    XX(18, UNBIND, UNBIND)           \
    XX(19, ACL, ACL)                 \
    /* subversion */                 \
    XX(20, REPORT, REPORT)           \
    XX(21, MKACTIVITY, MKACTIVITY)   \
    XX(22, CHECKOUT, CHECKOUT)       \
    XX(23, MERGE, MERGE)             \
    /* upnp */                       \
    XX(24, MSEARCH, M - SEARCH)      \
    XX(25, NOTIFY, NOTIFY)           \
    XX(26, SUBSCRIBE, SUBSCRIBE)     \
    XX(27, UNSUBSCRIBE, UNSUBSCRIBE) \
    /* RFC-5789 */                   \
    XX(28, PATCH, PATCH)             \
    XX(29, PURGE, PURGE)             \
    /* CalDAV */                     \
    XX(30, MKCALENDAR, MKCALENDAR)   \
    /* RFC-2068, section 19.6.1.2 */ \
    XX(31, LINK, LINK)               \
    XX(32, UNLINK, UNLINK)           \
    /* icecast */                    \
    XX(33, SOURCE, SOURCE)

enum http_method {
#define XX(num, name, string) HTTP_##name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    HTTP_INVALID_METHOD
};

http_method StringToHttpMethod(const std::string& str);
http_method CharsToHttpMethod(const char* str);
const char* HttpMethodToString(const http_method m);
const char* HttpStatusToString(const http_status s);

struct CmpInsensitiveLess {
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};

template<typename str_to_str, typename T>
bool checkGetAs(const str_to_str& m, const std::string& key, T& val, const T& def = T()) {
    auto it = m.find(key);
    if (it == m.end()) {
        val = def;
        return false;
    }

    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    }
    catch(...) {
        val = def;
    }
    return false;
}

template<typename str_to_str, typename T>
T getAs(const str_to_str& m, const std::string& key, const T& def = T()) {
    auto it = m.find(key);
    if (it == m.end()) {
        return def;
    }

    try {
        return boost::lexical_cast<T>(it->second);
    }
    catch(...) {
    }
    return def;
}

class HttpRequest {
public:
    using ptr = std::shared_ptr<HttpRequest>;
    using str_to_str = std::map<std::string, std::string, CmpInsensitiveLess>;

    HttpRequest(uint8_t version = 0x11, bool keep_alive = false);
    ~HttpRequest();

    http_method method() const { return method_; }
    uint8_t version() const { return version_; }
    bool keep_alive() const { return keep_alive_; }
    const std::string& path() const { return path_; }
    const std::string& query() const { return query_; }
    const std::string& body() const { return body_; }

    const str_to_str& headers() const { return headers_; }
    const str_to_str& params() const { return params_; }
    const str_to_str& cookies() const { return cookies_; }

    void set_method(const http_method m) { method_ = m; }
    void set_version(uint8_t v) { version_ = v; }
    void set_path(const std::string& path) { path_ = path; }
    void set_query(const std::string& query) { query_ = query; }
    void set_body(const std::string& body) { body_ = body; }
    void set_keep_alive(bool keep_alive) { keep_alive_ = keep_alive; }

    void set_headers(const str_to_str& headers) { headers_ = headers; }
    void set_params(const str_to_str& params) { params_ = params; }
    void set_cookies(const str_to_str& cookies) { cookies_ = cookies; }

    std::string header(const std::string& key, const std::string& def = "") const;
    std::string param(const std::string& key, const std::string& def = "") const;
    std::string cookie(const std::string& key, const std::string& def = "") const;

    void set_header(const std::string& key, const std::string& val);
    void set_param(const std::string& key, const std::string& val);
    void set_cookie(const std::string& key, const std::string& val);

    void del_header(const std::string& key);
    void del_param(const std::string& key);
    void del_cookie(const std::string& key);

    bool has_header(const std::string& key, std::string& val);
    bool has_param(const std::string& key, std::string& val);
    bool has_cookie(const std::string& key, std::string& val);

    template <typename T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(headers_, key, val, def);
    }

    template <typename T>
    T getHeaderAs(const std::string& key, const T& def = T()) {
        return getAs(headers_, key, def);
    }

    template <typename T>
    bool checkGetParamAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(params_, key, val, def);
    }

    template <typename T>
    T getParamAs(const std::string& key, const T& def = T()) {
        return getAs(params_, key, def);
    }

    template <typename T>
    bool checkGetCookieAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(cookies_, key, val, def);
    }

    template <typename T>
    T getCookieAs(const std::string& key, const T& def = T()) {
        return getAs(cookies_, key, def);
    }

    std::ostream& dump(std::ostream& os) const;
private:
    http_method method_;
    std::string path_;
    std::string query_;
    std::string fragment_;
    uint8_t version_;
    bool keep_alive_;
    str_to_str headers_;
    str_to_str params_;
    str_to_str cookies_;

    std::string body_;
};

class HttpResponse {
public:
    using ptr = std::shared_ptr<HttpResponse>;
    using str_to_str = std::map<std::string, std::string, CmpInsensitiveLess>;
    
    HttpResponse(uint8_t version = 0x11, bool keep_alive = false);
    ~HttpResponse();

    uint8_t version() const { return version_; }
    http_status status() const { return status_; }
    const std::string& reason() const { return reason_; }
    bool keep_alive() const { return keep_alive_; }
    const str_to_str& headers() const { return headers_; }
    const std::string& body() const { return body_; }

    void set_version(uint8_t version) { version_ = version; }
    void set_status(http_status status) { status_ = status; }
    void set_reason(const std::string& reason) { reason_ = reason; }
    void set_keep_alive(bool keep_alive) { keep_alive_ = keep_alive; }
    void set_headers(const str_to_str& headers) { headers_ = headers; }
    void set_body(const std::string& body) { body_ = body; }

    std::string header(const std::string& key, const std::string& def = "") const;
    void set_header(const std::string& key, const std::string& val);
    void del_header(const std::string& key, const std::string& val);

    template <typename T>
    bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
        return checkGetAs(headers_, key, val, def);
    }

    template <typename T>
    T getHeaderAs(const std::string& key, const T& def = T()) {
        return getAs(headers_, key, def);
    }

    std::ostream& dump(std::ostream& os) const;
private:
    uint8_t version_;
    http_status status_;
    std::string reason_;
    bool keep_alive_;
    
    str_to_str headers_;

    std::string body_;

};

}  // namespace http

}  // namespace tihi

#endif  // TIHI_HTTP_HTTP_H_