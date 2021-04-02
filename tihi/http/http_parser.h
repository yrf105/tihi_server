#ifndef TIHI_HTTP_HTTP_PARSER_H_
#define TIHI_HTTP_HTTP_PARSER_H_

#include "http.h"

namespace tihi {

namespace http {

class HttpRequestParser {
public:
    using ptr = std::shared_ptr<HttpRequestParser>;
    HttpRequestParser();

    size_t execute(char* data, size_t len);
    int has_error();
    int is_finished();

    HttpRequest::ptr data() { return data_; }
    void set_error(int error) { error_ = error; }
    uint64_t content_length() const;
public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBufferSize();

private:
    http_parser parser_;
    HttpRequest::ptr data_;
    int error_;
};

class HttpResponseParser {
public:
    using ptr = std::shared_ptr<HttpResponseParser>;

    HttpResponseParser();

    size_t execute(char *data, size_t len);
    int has_error();
    int is_finished();

    HttpResponse::ptr data() const { return data_; }
    void set_error(int error) { error_ = error; }
    uint64_t content_length() const;

private:
    httpclient_parser parser_;
    HttpResponse::ptr data_;
    int error_;
};

} // namespace http

} // namesapce tihi

#endif // TIHI_HTTP_HTTP_PARSER_H_