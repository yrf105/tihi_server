#include "http/http.h"
#include "log/log.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void test() {
    tihi::http::HttpRequest::ptr req(new tihi::http::HttpRequest);
    req->set_method(tihi::http::http_method::HTTP_GET);
    req->set_version(0x11);
    req->set_path("/");
    req->set_keep_alive(true);
    req->set_param("cc", "8");
    req->set_header("host", "www.baidu.com");
    req->set_body("hello world");
    
    req->dump(std::cout) << std::endl;

    tihi::http::HttpResponse::ptr res(new tihi::http::HttpResponse);
    res->set_status(tihi::http::http_status::HTTP_STATUS_OK);
    res->set_body("hello world");
    res->dump(std::cout) << std::endl;

}

int main(int argc, char** argv) {
    test();
    return 0;
}