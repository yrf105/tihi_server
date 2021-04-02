#include "http_session.h"

namespace tihi {

namespace http {

HttpSession::HttpSession(Socket::ptr socket, bool owner)
    : SocketStream(socket, owner) {}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    // uint64_t buff_size = 150;

    std::shared_ptr<char> buffer(new char[buff_size],
                                 [](char* ptr) { delete[] ptr; });

    char* data = buffer.get();
    size_t offset = 0;
    do {

        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            return nullptr;
        }

        len += offset;
        size_t nparse = parser->execute(data, len);
        if (parser->has_error()) {
            return nullptr;
        }

        offset = len - nparse;
        if (offset == buff_size) {
            return nullptr;
        }

        if (parser->is_finished()) {
            break;
        }
    } while (true);
    int64_t lenght = parser->content_length();
    while (lenght > 0) {
        std::string body;
        body.reserve(lenght);

        if (lenght >= (int64_t)offset) {
            body.append(data, offset);
        } else {
            body.append(data, lenght);
        }

        lenght -= offset;

        if (lenght > 0) {
            if (readFixSize(&body[body.size()], lenght) <= 0) {
                return nullptr;
            }
        }

        parser->data()->set_body(body);
    }

    return parser->data();
}

int HttpSession::sendResponse(HttpResponse::ptr res) {
    std::stringstream ss;
    ss << *res;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}  // namespace http

}  // namespace tihi