#include "http.h"
#include "log/log.h"

namespace tihi {

namespace http {

static tihi::Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

http_method StringToHttpMethod(const std::string& str) {
#define XX(num, name, string)              \
    if (strcmp(str.c_str(), #name) == 0) { \
        return http_method::HTTP_##name;   \
    }

    HTTP_METHOD_MAP(XX)
#undef XX

    return http_method::HTTP_INVALID_METHOD;
}

http_method CharsToHttpMethod(const char* str) {
#define XX(num, name, string)            \
    if (strncmp(str, #name, strlen(#name)) == 0) {       \
        return http_method::HTTP_##name; \
    }

    HTTP_METHOD_MAP(XX)
#undef XX

    return http_method::HTTP_INVALID_METHOD;
}

static const char* s_method_to_string[] = {
#define XX(num, name, string) #name,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(const http_method m) {
    if (m >= sizeof(s_method_to_string) / sizeof(s_method_to_string[0])) {
        return "<unknown>";
    }
    return s_method_to_string[m];
}

const char* HttpStatusToString(const http_status s) {
    switch ((uint32_t)s) {
#define XX(code, name, msg)        \
    case http_status::HTTP_STATUS_##name: \
        return #name;              

    HTTP_STATUS_MAP(XX)
#undef XX
    }
    return "<unknown>";
}

bool CmpInsensitiveLess::operator()(const std::string& lhs,
                                    const std::string& rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool keep_alive)
    : method_(http_method::HTTP_GET),
      path_("/"),
      version_(version),
      keep_alive_(keep_alive) {
    // headers_["connection"] = (keep_alive_ ? "heep-alive" : "close");
}

HttpRequest::~HttpRequest() {}

std::string HttpRequest::header(const std::string& key,
                                const std::string& def) const {
    auto it = headers_.find(key);
    return it == headers_.end() ? def : it->second;
}

std::string HttpRequest::param(const std::string& key,
                               const std::string& def) const {
    auto it = params_.find(key);
    return it == params_.end() ? def : it->second;
}

std::string HttpRequest::cookie(const std::string& key,
                                const std::string& def) const {
    auto it = cookies_.find(key);
    return it == cookies_.end() ? def : it->second;
}

void HttpRequest::set_header(const std::string& key, const std::string& val) {
    headers_[key] = val;
}

void HttpRequest::set_param(const std::string& key, const std::string& val) {
    params_[key] = val;
}

void HttpRequest::set_cookie(const std::string& key, const std::string& val) {
    cookies_[key] = val;
}

void HttpRequest::del_header(const std::string& key) { headers_.erase(key); }

void HttpRequest::del_param(const std::string& key) { params_.erase(key); }

void HttpRequest::del_cookie(const std::string& key) { cookies_.erase(key); }

bool HttpRequest::has_header(const std::string& key, std::string& val) {
    auto it = headers_.find(key);
    if (it == headers_.end()) {
        return false;
    }
    val = it->second;

    return true;
}

bool HttpRequest::has_param(const std::string& key, std::string& val) {
    auto it = params_.find(key);
    if (it == params_.end()) {
        return false;
    }
    val = it->second;

    return true;
}

bool HttpRequest::has_cookie(const std::string& key, std::string& val) {
    auto it = cookies_.find(key);
    if (it == cookies_.end()) {
        return false;
    }
    val = it->second;

    return true;
}

std::ostream& HttpRequest::dump(std::ostream& os) const {
    os << HttpMethodToString(method_) << " " << path_
       << (query_.empty() ? "" : "?") << query_
       << (fragment_.empty() ? "" : "#") << fragment_ << " HTTP/"
       << ((uint32_t)(version_ >> 4)) << "." << ((uint32_t)(version_ & 0x0f))
       << "\r\n";

    os << "connection:" << (keep_alive_ ? "keep-alive" : "close") << "\r\n";

    for (const auto& h : headers_) {
        if (strcasecmp(h.first.c_str(), "connection") == 0) {
            continue;
        }
        os << h.first << ":" << h.second << "\r\n";
    }

    if (!body_.empty()) {
        os << "content-length:" << body_.size() << "\r\n\r\n" << body_;
    } else {
        os << "\r\n" << body_;
    }

    return os;
}

const std::string HttpRequest::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

HttpResponse::HttpResponse(uint8_t version, bool keep_alive)
    : version_(version),
      status_(http_status::HTTP_STATUS_OK),
      keep_alive_(keep_alive) {}

HttpResponse::~HttpResponse() {}

std::string HttpResponse::header(const std::string& key,
                                 const std::string& def) const {
    auto it = headers_.find(key);
    return it == headers_.end() ? def : it->second;
}

void HttpResponse::set_header(const std::string& key, const std::string& val) {
    headers_[key] = val;
}

void HttpResponse::del_header(const std::string& key, const std::string& val) {
    headers_.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream& os) const {
    os << "HTTP/" << (uint32_t)(version_ >> 4) << "."
       << (uint32_t)(version_ & 0x0f) << " " << (uint32_t)status_ << " "
       << (reason_.empty() ? HttpStatusToString(status_) : reason_) << "\r\n";

    os << "connection:" << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
    for (const auto& h : headers_) {
        if (strcasecmp("connection", h.first.c_str()) == 0) {
            continue;
        }
        os << h.first << ":" << h.second << "\r\n";
    }

    if (!body_.empty()) {
        os << "content-length:" << body_.size() << "\r\n\r\n" << body_;
    } else {
        os << "\r\n";
    }

    return os;
}

const std::string HttpResponse::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req) {
    return req.dump(os);
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& res) {
    return res.dump(os);
}

}  // namespace http

}  // namespace tihi