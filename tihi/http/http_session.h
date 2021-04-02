#ifndef TIHI_HTTP_HTTP_SESSION_H_
#define TIHI_HTTP_HTTP_SESSION_H_

#include <memory>

#include "socket/stream/socket_stream.h"
#include "http.h"
#include "http_parser.h"

namespace tihi {

namespace http {

class HttpSession : public tihi::SocketStream {
public:
    using ptr = std::shared_ptr<HttpSession>;
    HttpSession(Socket::ptr socket, bool owner = true);

    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr res);

private:
};

} // namespace tihi

} // namespace tihi

#endif // TIHI_HTTP_HTTP_SESSION_H_