#ifndef TIHI_HTTP_HTTP_CONNECTION_H_
#define TIHI_HTTP_HTTP_CONNECTION_H_

#include <atomic>
#include <list>
#include <memory>

#include "http.h"
#include "http_parser.h"
#include "socket/stream/socket_stream.h"
#include "uri/uri.h"
#include "utils/mutex.h"


namespace tihi {

namespace http {

struct HttpResult {
    using ptr = std::shared_ptr<HttpResult>;
    enum Error {
        OK = 0,
        INVALID_URL = 1,
        INVALID_HOST = 2,
        CONNECT_FAIL = 3,
        SEND_CLOSED_BY_PEER = 4,
        SEND_SOCKET_ERROR = 5,
        TIMEOUT = 6,
        POOL_CONNECTION_FAIL = 6,
        POOL_INVALID_CONNECTION = 7,

    };

    HttpResult(int _result, HttpResponse::ptr _response,
               const std::string& _error)
        : result(_result), response(_response), error(_error) {}

    std::string toString() const;

    int result;
    HttpResponse::ptr response;
    std::string error;
};

class HttpConnectionPool;

class HttpConnection : public tihi::SocketStream {
friend HttpConnectionPool;
public:
    using ptr = std::shared_ptr<HttpConnection>;
    HttpConnection(Socket::ptr socket, bool owner = true);
    ~HttpConnection();

    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);

    static HttpResult::ptr DoGet(
        const std::string& url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");

    static HttpResult::ptr DoGet(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");

    static HttpResult::ptr DoPost(
        const std::string& url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");
    static HttpResult::ptr DoPost(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");

    static HttpResult::ptr DoRequest(
        http_method method, const std::string& url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");
    static HttpResult::ptr DoRequest(
        http_method method, Uri::ptr url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");
    static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri,
                                     uint64_t timeout_ms);

private:
    uint64_t m_createTime = 0;
    uint32_t m_request = 0;
};

class HttpConnectionPool {
public:
    using ptr = std::shared_ptr<HttpConnectionPool>;
    using mutex_type = Mutex;

    HttpConnectionPool(const std::string& host, const std::string& vhost,
                       uint32_t port, uint32_t max_size,
                       uint32_t max_alive_time, uint32_t max_request);
    HttpConnection::ptr connection();

    HttpResult::ptr doGet(
        const std::string& url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");
    HttpResult::ptr doGet(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");

    HttpResult::ptr doPost(
        const std::string& url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");
    HttpResult::ptr doPost(
        Uri::ptr uri, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");

    HttpResult::ptr doRequest(
        http_method method, const std::string& url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");
    HttpResult::ptr doRequest(
        http_method method, Uri::ptr url, uint64_t timeout_ms,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = "");
    HttpResult::ptr doRequest(HttpRequest::ptr req,
                                     uint64_t timeout_ms);

private:
    static void RelasePtr(HttpConnection* ptr, HttpConnectionPool* pool);
    // struct HttpConnectionInfo {
    //     HttpConnection* connection;
    //     uint64_t create_time;
    // };
private:
    std::string m_host;
    std::string m_vhost;
    uint32_t m_port;
    uint32_t m_maxSize;
    uint32_t m_maxAliveTime;
    uint32_t m_maxRequest;

    std::list<HttpConnection*> m_pool;
    std::atomic<uint32_t> m_total = {0};

    mutex_type m_mutex;
};

}  // namespace http

}  // namespace tihi

#endif  // TIHI_HTTP_HTTP_CONNECTION_H_