#include "http_connection.h"

#include "log/log.h"

namespace tihi {

namespace http {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult: result=" << result
       << " respones=" << (response ? response->toString() : "nullptr") << " error=" << error;
    return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr socket, bool owner)
    : SocketStream(socket, owner) {}

HttpConnection::~HttpConnection() {
    TIHI_LOG_DEBUG(g_sys_logger) << "HttpConnection::~HttpConnection";
}

HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize() +
                         HttpResponseParser::GetHttpResponseMaxBufferSize();
    // uint64_t buff_size = 150;

    std::shared_ptr<char> buffer(new char[buff_size + 1],
                                 [](char* ptr) { delete[] ptr; });

    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            close();
            return nullptr;
        }

        len += offset;
        data[len] = '\0';
        size_t nparse = parser->execute(data, len);
        if (parser->has_error()) {
            close();
            return nullptr;
        }

        offset = len - nparse;
        if (offset == (int)buff_size) {
            close();
            return nullptr;
        }

        if (parser->is_finished()) {
            break;
        }
    } while (true);

    auto client_parser = parser->paraer();
    if (client_parser.chunked) {
        std::string body;
        int len = offset;
        do {
            do {
                int rt = read(data + len, buff_size - len);
                if (rt <= 0) {
                    close();
                    return nullptr;
                }
                len += rt;
                data[len] = '\0';
                TIHI_LOG_DEBUG(g_sys_logger) << "len: " << len;

                size_t nparse = parser->execute(data, len, true);
                if (parser->has_error()) {
                    close();
                    return nullptr;
                }
                TIHI_LOG_DEBUG(g_sys_logger) << "nparse: " << nparse;
                len -= nparse;
                if (len == (int)buff_size) {
                    close();
                    return nullptr;
                }
            } while (!parser->is_finished());
            len -= 2;

            TIHI_LOG_DEBUG(g_sys_logger)
                << "content-length: " << client_parser.content_len
                << " len: " << len;

            if (client_parser.content_len >= 0 &&
                client_parser.content_len <= len) {
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len,
                        len - client_parser.content_len);
            } else {
                body.append(data, len);
                int left = client_parser.content_len - len;
                while (left > 0) {
                    int rt =
                        read(data, left > (int)buff_size ? buff_size : left);
                    if (rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                len = 0;
            }

        } while (!client_parser.chunks_done);
        parser->data()->set_body(body);
    } else {
        int64_t length = parser->content_length();
        while (length > 0) {
            std::string body;
            body.resize(length);

            int len = 0;
            if (length >= offset) {
                // body.append(data, offset);
                memcpy(&body[0], data, offset);
                len = offset;
            } else {
                // body.append(data, length);
                memcpy(&body[0], data, length);
                len = length;
            }

            length -= offset;

            if (length > 0) {
                int rt = readFixSize(&body[len], length);
                // TIHI_LOG_DEBUG(g_sys_logger) << "rt: " << rt << " length: "
                // << length;
                if (rt <= 0) {
                    close();
                    return nullptr;
                }
                length = 0;
            }

            parser->data()->set_body(body);
        }
    }

    return parser->data();
}

int HttpConnection::sendRequest(HttpRequest::ptr req) {
    std::stringstream ss;
    ss << *req;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

HttpResult::ptr HttpConnection::DoGet(
    const std::string& url, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url:" + url);
    }
    return DoGet(uri, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnection::DoGet(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    return DoRequest(http_method::HTTP_GET, uri, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnection::DoPost(
    const std::string& url, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url:" + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    return DoRequest(http_method::HTTP_POST, uri, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnection::DoRequest(
    http_method method, const std::string& url, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    Uri::ptr uri = Uri::Create(url);
    if (!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,
                                            nullptr, "invalid url:" + url);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(
    http_method method, Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->set_method(method);
    req->set_path(uri->path());
    req->set_query(uri->query());
    req->set_fragment(uri->fragment());
    bool has_host = false;
    for (auto h : headers) {
        if (strcasecmp(h.first.c_str(), "connection") == 0) {
            if (strcasecmp(h.second.c_str(), "keep-alive") == 0) {
                req->set_keep_alive(true);
            }
            continue;
        }
        if (!has_host && strcasecmp(h.first.c_str(), "host") == 0) {
            has_host = !(h.second.empty());
        }

        req->set_header(h.first, h.second);
    }

    if (!has_host) {
        req->set_header("host", uri->host());
    }
    req->set_body(body);

    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, Uri::ptr uri,
                                          uint64_t timeout_ms) {
    Address::ptr addr = uri->createAddress();
    if (!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::INVALID_HOST,
                                            nullptr,
                                            "invalid host: " + uri->host());
    }

    Socket::ptr sock = Socket::TCPSocket(addr);
    if (!sock) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::CONNECT_FAIL, nullptr,
            "connect fail: addr: " + addr->toString());
    }

    int rt = sock->connect(addr);
    if (!rt) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::CONNECT_FAIL, nullptr,
            "connect fail: addr: " + addr->toString());
    }

    sock->set_recv_timeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    rt = conn->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::SEND_CLOSED_BY_PEER, nullptr,
            "send closed by peer: addr: " + addr->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::SEND_SOCKET_ERROR, nullptr,
            "send socket error: addr: " + addr->toString() +
                " errno: " + std::to_string(errno) +
                " strerror: " + std::string(strerror(errno)));
    }

    HttpResponse::ptr rsp = conn->recvResponse();
    if (!rsp) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::TIMEOUT, nullptr,
            "timeout: " + addr->toString() +
                " errno: " + std::to_string(errno) +
                " strerror: " + std::string(strerror(errno)));
    }

    return std::make_shared<HttpResult>((int)HttpResult::OK, rsp, "OK");
}

HttpConnectionPool::HttpConnectionPool(const std::string& host,
                                       const std::string& vhost, uint32_t port,
                                       uint32_t max_size,
                                       uint32_t max_alive_time,
                                       uint32_t max_request)
    : m_host(host),
      m_vhost(vhost),
      m_port(port),
      m_maxSize(max_size),
      m_maxAliveTime(max_alive_time),
      m_maxRequest(max_request) {}

HttpConnection::ptr HttpConnectionPool::connection() {
    uint64_t now_ms = tihi::MS();
    std::vector<HttpConnection*> invalid_conn;
    HttpConnection* ptr = nullptr;

    mutex_type::mutex lock(m_mutex);
    while (!m_pool.empty()) {
        HttpConnection* conn = *(m_pool.begin());
        m_pool.pop_front();

        if (!conn->isConnected()) {
            invalid_conn.push_back(conn);
            continue;
        }
        if ((conn->m_createTime + m_maxAliveTime) > now_ms) {
            invalid_conn.push_back(conn);
            continue;
        }

        ptr = conn;
        break;
    }

    lock.unlock();
    for (auto i : invalid_conn) {
        delete i;
    }
    m_total -= invalid_conn.size();

    if (!ptr) {
        Address::ptr addr = Address::LookUpIPAddress(m_host);
        if (!addr) {
            TIHI_LOG_ERROR(g_sys_logger) << "lookup addr fail: host" << m_host;
            return nullptr;
        }
        addr->set_port(m_port);

        Socket::ptr sock = Socket::TCPSocket(addr);
        if (!sock) {
            TIHI_LOG_ERROR(g_sys_logger)
                << "create tcp socket fail: addr: " << *addr;
            return nullptr;
        }

        int rt = sock->connect(addr);
        if (!rt) {
            TIHI_LOG_ERROR(g_sys_logger) << "connect fail: addr: " << *addr;
            return nullptr;
        }

        ptr = new HttpConnection(sock);
        ++m_total;
    }

    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::RelasePtr,
                                              std::placeholders::_1, this));
}


HttpResult::ptr HttpConnectionPool::doGet(
    const std::string& url, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    return doRequest(http_method::HTTP_GET, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    /**
     * 拿出除 host 外的其他部分
     */
    std::stringstream ss;
    ss << uri->path() << (uri->query().empty() ? "" : "?") << uri->query()
       << (uri->fragment().empty() ? "" : "#") << uri->fragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}


HttpResult::ptr HttpConnectionPool::doPost(
    const std::string& url, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    return doRequest(http_method::HTTP_POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(
    Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    /**
     * 拿出除 host 外的其他部分
     */
    std::stringstream ss;
    ss << uri->path() << (uri->query().empty() ? "" : "?") << uri->query()
       << (uri->fragment().empty() ? "" : "#") << uri->fragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}


HttpResult::ptr HttpConnectionPool::doRequest(
    http_method method, const std::string& url, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->set_keep_alive(true);
    req->set_method(method);
    req->set_path(url);
    bool has_host = false;
    for (auto h : headers) {
        if (strcasecmp(h.first.c_str(), "connection") == 0) {
            if (strcasecmp(h.second.c_str(), "keep-alive") == 0) {
                req->set_keep_alive(true);
            }
            continue;
        }
        if (!has_host && strcasecmp(h.first.c_str(), "host") == 0) {
            has_host = !(h.second.empty());
        }

        req->set_header(h.first, h.second);
    }

    if (!has_host) {
        if (!m_vhost.empty()) {
            req->set_header("host", m_vhost);
        } else {
            req->set_header("host", m_host);
        }
    }
    req->set_body(body);

    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(
    http_method method, Uri::ptr uri, uint64_t timeout_ms,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    /**
     * 拿出除 host 外的其他部分
     */
    std::stringstream ss;
    ss << uri->path() << (uri->query().empty() ? "" : "?") << uri->query()
       << (uri->fragment().empty() ? "" : "#") << uri->fragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req,
                                              uint64_t timeout_ms) {
    HttpConnection::ptr conn = connection();
    if (!conn) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::POOL_CONNECTION_FAIL, nullptr,
            "pool connection fail: pool-host: " + m_host +
                " pool-port: " + std::to_string(m_port));
    }

    Socket::ptr sock = conn->socket();
    if (!sock) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::POOL_INVALID_CONNECTION, nullptr,
            "pool connection fail: pool-host: " + m_host +
                " pool-port: " + std::to_string(m_port));
    }
    Address::ptr addr = sock->remote_addr();

    sock->set_recv_timeout(timeout_ms);
    int rt = conn->sendRequest(req);
    if (rt == 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::SEND_CLOSED_BY_PEER, nullptr,
            "send closed by peer: addr: " + addr->toString());
    }
    if (rt < 0) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::SEND_SOCKET_ERROR, nullptr,
            "send socket error: addr: " + addr->toString() +
                " errno: " + std::to_string(errno) +
                " strerror: " + std::string(strerror(errno)));
    }
    HttpResponse::ptr rsp = conn->recvResponse();
    if (!rsp) {
        return std::make_shared<HttpResult>(
            (int)HttpResult::TIMEOUT, nullptr,
            "timeout: " + addr->toString() +
                " errno: " + std::to_string(errno) +
                " strerror: " + std::string(strerror(errno)));
    }
    return std::make_shared<HttpResult>((int)HttpResult::OK, rsp, "OK");
}

void HttpConnectionPool::RelasePtr(HttpConnection* ptr,
                                   HttpConnectionPool* pool) {
    ++(ptr->m_request);
    if (!ptr->isConnected() || ptr->m_request > pool->m_maxRequest ||
        ptr->m_createTime + pool->m_maxAliveTime > tihi::MS()) {
        delete ptr;
        --(pool->m_total);
        return;
    }
    mutex_type::mutex lock(pool->m_mutex);
    pool->m_pool.push_back(ptr);
}


}  // namespace http

}  // namespace tihi