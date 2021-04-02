#include "http_server.h"

#include "log/log.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

namespace http {

HttpServer::HttpServer(bool keep_alive, IOManager* worker,
                       IOManager* accept_worker)
    : TcpServer(worker, accept_worker) , m_keep_alive(keep_alive){
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr sock) {
    HttpSession::ptr session(new HttpSession(sock));

    do {
        auto req = session->recvRequest();
        if (!req) {
            TIHI_LOG_WARN(g_sys_logger)
                << "recv http request fail. errno=" << errno
                << " strerror=" << strerror(errno) << " sock=" << *sock;
        }

        HttpResponse::ptr rsp(new HttpResponse(req->version(), req->keep_alive() && m_keep_alive));
        m_dispatch->handle(req, rsp, session);
        // rsp->set_body("hello world");

        /**
         * 不在 handle 里直接发送相应，这样可以发送前更自由地添加处理信息
        */
        session->sendResponse(rsp);
    } while (m_keep_alive);
}

}  // namespace http
}  // namespace tihi