#include "http/http_parser.h"
#include "log/log.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

const char http_request[] =
    "POST / HTTP/1.1\r\n"
    "host:www.baidu.com\r\n"
    "content-length: 10\r\n"
    "\r\n"
    "1234567890";

void test() {
    tihi::http::HttpRequestParser::ptr res_parser(
        new tihi::http::HttpRequestParser);
    std::string tmp = http_request;
    size_t rt = res_parser->execute(&(tmp[0]), tmp.size());
    TIHI_LOG_INFO(g_logger)
        << "rt=" << rt << " error=" << res_parser->has_error()
        << " finish=" << res_parser->is_finished() << " size=" << tmp.size()
        << " content-length=" << res_parser->content_length();

    tmp.resize(tmp.size() - rt);
    TIHI_LOG_INFO(g_logger) << res_parser->data()->toString() << tmp;
}

const char http_response[] =
    "HTTP/1.1 302 Moved Temporarily\r\n"
    "Server: ias/1.3.5_1.17.3\r\n"
    "Date: Wed, 31 Mar 2021 08:04:50 GMT\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 149\r\n"
    "Connection: close\r\n"
    "Location: https://www.qq.com/\r\n"
    "\r\n"
    "<html>\r\n"
    "<head><title>302 Found</title></head>\r\n"
    "<body>\r\n"
    "<center><h1>302 Found</h1></center>\r\n"
    "<hr><center>ias/1.3.5_1.17.3</center>\r\n"
    "</body>\r\n"
    "</html>";

void test_response() {
    tihi::http::HttpResponseParser::ptr resp_parser(
        new tihi::http::HttpResponseParser);
    std::string tmp = http_response;
    int rt = resp_parser->execute(&(tmp[0]), tmp.size());
    TIHI_LOG_INFO(g_logger)
        << "rt=" << rt << " error=" << resp_parser->has_error()
        << " finish=" << resp_parser->is_finished()
        << " content-lenght=" << resp_parser->content_length();

    tmp.resize(tmp.size() - rt);
    TIHI_LOG_INFO(g_logger) << std::endl << resp_parser->data()->toString() << tmp;
}

int main(int argc, char** argv) {
    test();
    test_response();
    return 0;
}