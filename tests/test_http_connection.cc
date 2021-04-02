#include "http/http_connection.h"
#include "log/log.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void test_pool() {
    tihi::http::HttpConnectionPool::ptr pool(new tihi::http::HttpConnectionPool(
        "www.sylar.top", "", 80, 10, 30 * 1000, 5));

    tihi::IOManager::This()->addTimer(1000, [pool]() {
        auto r = pool->doGet("/", 300);
        TIHI_LOG_INFO(g_logger) << r->toString();
    }, true);
}

void run() {
    tihi::Address::ptr addr =
        tihi::Address::LookUpIPAddress("www.baidu.com:80");
    if (!addr) {
        TIHI_LOG_DEBUG(g_logger) << "get addr error";
        return;
    }

    tihi::Socket::ptr sock = tihi::Socket::TCPSocket(addr);
    bool rt = sock->connect(addr);
    if (!rt) {
        TIHI_LOG_DEBUG(g_logger)
            << "connect error rt = " << rt << " errno: " << errno
            << " strerror: " << strerror(errno);
        return;
    }

    tihi::http::HttpConnection::ptr conn(new tihi::http::HttpConnection(sock));
    tihi::http::HttpRequest::ptr req(new tihi::http::HttpRequest);
    // req->set_path("/blog/");
    // req->set_header("host", "www.sylar.top");
    TIHI_LOG_DEBUG(g_logger) << "req: " << std::endl << *req;
    conn->sendRequest(req);
    auto rsp = conn->recvResponse();
    if (!rsp) {
        TIHI_LOG_DEBUG(g_logger) << "recv error";
        return;
    }
    TIHI_LOG_DEBUG(g_logger) << "rsp: " << std::endl << *rsp;

    TIHI_LOG_DEBUG(g_logger) << "===========================";
    auto r = tihi::http::HttpConnection::DoGet("http://www.baidu.com/", 300);
    TIHI_LOG_DEBUG(g_logger)
        << "result: " << r->result << " error: " << r->error
        << " r: " << (r->response ? r->response->toString() : "");

    TIHI_LOG_DEBUG(g_logger) << "===========================";
    test_pool();
}

int main(int argc, char** argv) {
    tihi::IOManager iom;
    iom.schedule(&run);

    return 0;
}