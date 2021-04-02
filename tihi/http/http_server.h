#ifndef TIHI_HTTP_HTTP_SERVER_H_
#define TIHI_HTTP_HTTP_SERVER_H_

#include "http_session.h"
#include "tcp_server/tcp_server.h"
#include "servlet.h"

namespace tihi {

namespace http {

class HttpServer : public tihi::TcpServer {
public:
    using ptr = std::shared_ptr<HttpServer>;
    HttpServer(bool keep_alive = false, IOManager* worker = IOManager::This(),
               IOManager* accept_worker = IOManager::This());

    ServletDispatch::ptr dispatch() { return m_dispatch; }
    void set_dispatch(ServletDispatch::ptr dispatch) { m_dispatch = dispatch; }

protected:
    virtual void handleClient(Socket::ptr sock) override;

private:
    bool m_keep_alive;
    ServletDispatch::ptr m_dispatch;
};

}  // namespace http

}  // namespace tihi

#endif  // TIHI_HTTP_HTTP_SERVER_H_