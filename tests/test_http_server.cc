#include "http/http_server.h"

void run() {
    tihi::http::HttpServer::ptr http_server(new tihi::http::HttpServer);
    tihi::Address::ptr addr = tihi::Address::LookUpAny("0.0.0.0:12345");
    while (!http_server->bind(addr)) {
        sleep(2);
    }
    auto dispatch = http_server->dispatch();
    dispatch->addServlet("/tihi", [](tihi::http::HttpRequest::ptr req,
                                    tihi::http::HttpResponse::ptr rsp,
                                    tihi::http::HttpSession::ptr session) {
        rsp->set_body(req->toString());
        return 0;
    });
    dispatch->addGlobServlet("/tihi/*", [](tihi::http::HttpRequest::ptr req,
                                    tihi::http::HttpResponse::ptr rsp,
                                    tihi::http::HttpSession::ptr session) {
        rsp->set_body("Glob\r\n" + req->toString());
        return 0;
    });
    http_server->start();
}

int main(int argc, char** argv) {
    tihi::IOManager iom;
    iom.schedule(&run);

    return 0;
}