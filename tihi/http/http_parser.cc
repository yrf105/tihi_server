#include "http_parser.h"
#include "log/log.h"
#include "config/config.h"

namespace tihi {
namespace http {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");
static tihi::ConfigVar<uint64_t>::ptr g_http_request_buffer_size = 
    Config::Lookup<uint64_t>("http.request.buffer_size", 4 * 1024ul, "http request buffer size");
static tihi::ConfigVar<uint64_t>::ptr g_http_request_max_body_size = 
    Config::Lookup<uint64_t>("http.request.max_body_size", 64 * 1024ul * 1024ul, "http request max body size");

static tihi::ConfigVar<uint64_t>::ptr g_http_response_buffer_size = 
    Config::Lookup<uint64_t>("http.response.buffer_size", 4 * 1024ul, "http response buffer size");
static tihi::ConfigVar<uint64_t>::ptr g_http_response_max_body_size = 
    Config::Lookup<uint64_t>("http.response.max_body_size", 64 * 1024ul * 1024ul, "http response max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_buffer_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_buffer_size = 0;

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBufferSize() {
    return s_http_request_max_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBufferSize() {
    return s_http_response_max_buffer_size;
}

namespace {

struct _RequestSizeIniter {
    _RequestSizeIniter() {
        s_http_request_buffer_size = g_http_request_buffer_size->value();
        s_http_request_max_buffer_size = g_http_request_max_body_size->value();
        s_http_response_buffer_size = g_http_response_buffer_size->value();
        s_http_response_max_buffer_size = g_http_response_max_body_size->value();

        g_http_request_buffer_size->addListener([](const uint64_t& old_val, const uint64_t& new_val){
            s_http_request_buffer_size = new_val;
        });

        g_http_request_max_body_size->addListener([](const uint64_t& old_val, const uint64_t& new_val){
            s_http_request_max_buffer_size = new_val;
        });

        g_http_response_buffer_size->addListener([](const uint64_t& old_val, const uint64_t& new_val){
            s_http_response_buffer_size = new_val;
        });

        g_http_response_max_body_size->addListener([](const uint64_t& old_val, const uint64_t& new_val){
            s_http_response_max_buffer_size = new_val;
        });
    }
};

static _RequestSizeIniter _init;

}

void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    http_method m = CharsToHttpMethod(at);

    if (m == http_method::HTTP_INVALID_METHOD) {
        TIHI_LOG_WARN(g_sys_logger) << "invalid http method";
        parser->set_error(1000);
        return ;
    }

    parser->data()->set_method(m);
}

void on_request_uri(void *data, const char *at, size_t length) {

}

void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->data()->set_fragment(std::string(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->data()->set_path(std::string(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->data()->set_query(std::string(at, length));
}

void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;

    if (strncasecmp("HTTP/1.1", at, length) == 0) {
        v = 0x11;
    } else if (strncasecmp("HTTP/1.0", at, length) == 0) {
        v = 0x10;
    } else {
        TIHI_LOG_WARN(g_sys_logger) << "invalid http version";
        parser->set_error(1001);
        return ;
    }

    parser->data()->set_version(v);
}

void on_request_header_done(void *data, const char *at, size_t length) {
    // HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);

}

void on_request_http_filed(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if (vlen == 0) {
        TIHI_LOG_WARN(g_sys_logger) << "file vlen = 0";
        // parser->set_error(1002);
        return ;
    }
    parser->data()->set_header(std::string(field, flen), std::string(value, vlen));

}

HttpRequestParser::HttpRequestParser() : error_(0) {
    http_parser_init(&parser_);
    data_.reset(new HttpRequest);
    parser_.request_method = on_request_method;
    parser_.request_uri = on_request_uri;
    parser_.fragment = on_request_fragment;
    parser_.request_path = on_request_path;
    parser_.query_string = on_request_query;
    parser_.http_version = on_request_version;
    parser_.header_done = on_request_header_done;
    parser_.http_field = on_request_http_filed;
    parser_.data = this;
}

size_t HttpRequestParser::execute(char *data, size_t len) {
    size_t offset = http_parser_execute(&parser_, data, len, 0);
    memmove(data, data + offset, len - offset);

    return offset;
}

int HttpRequestParser::has_error() {
    return error_ || http_parser_has_error(&parser_);
}

int HttpRequestParser::is_finished() {
    return http_parser_is_finished(&parser_);
}

uint64_t HttpRequestParser::content_length() const {
    return data_->getHeaderAs<int>("content-length", 0);
}


void on_response_reason_phrase(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->data()->set_reason(std::string(at, length));
}

void on_response_status_code(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    http_status s = (http_status)(atoi(at));
    parser->data()->set_status(s);
}

void on_response_chunk_size(void *data, const char *at, size_t length) {
    // HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    
}

void on_response_http_version(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;

    if (strncasecmp("HTTP/1.1", at, length) == 0) {
        v = 0x11;
    } else if (strncasecmp("HTTP/1.0", at, length) == 0) {
        v = 0x10;
    } else {
        TIHI_LOG_WARN(g_sys_logger) << "invalid http version";
        parser->set_error(1001);
        return ;
    }

    parser->data()->set_version(v);
}

void on_response_header_done(void *data, const char *at, size_t length) {
    // HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);

}

void on_response_last_chunk(void *data, const char *at, size_t length) {
    // HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);

}

void on_reponse_http_filed(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if (vlen == 0) {
        TIHI_LOG_WARN(g_sys_logger) << "file vlen = 0";
        // parser->set_error(1002);
        return ;
    }
    parser->data()->set_header(std::string(field, flen), std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser() : error_(0) {
    httpclient_parser_init(&parser_);
    data_.reset(new HttpResponse);
    parser_.reason_phrase =  on_response_reason_phrase;
    parser_.status_code =  on_response_status_code;
    parser_.chunk_size =  on_response_chunk_size;
    parser_.http_version =  on_response_http_version;
    parser_.header_done =  on_response_header_done;
    parser_.last_chunk =  on_response_last_chunk;
    parser_.http_field = on_reponse_http_filed;
    parser_.data = this;
}

size_t HttpResponseParser::execute(char *data, size_t len, bool chunked) {
    if (chunked) {
        ::httpclient_parser_init(&parser_);
    }
    size_t offset = httpclient_parser_execute(&parser_, data, len, 0);
    memmove(data, data + offset, len - offset);

    return offset;
}

int HttpResponseParser::has_error() {
    return error_ || httpclient_parser_has_error(&parser_);
}

int HttpResponseParser::is_finished() {
    return httpclient_parser_finish(&parser_);
}

uint64_t HttpResponseParser::content_length() const {
    return data_->getHeaderAs<int>("content-length", 0);
}

} // namespace http

} // namesapce tihi