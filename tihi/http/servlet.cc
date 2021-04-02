#include "servlet.h"

#include <fnmatch.h>

namespace tihi {
namespace http {

FunctionServlet::FunctionServlet(callback cb)
    : Servlet("FunctionServlet"), m_cb(cb) {}

int32_t FunctionServlet::handle(HttpRequest::ptr request,
                                HttpResponse::ptr response,
                                HttpSession::ptr session) {
    return m_cb(request, response, session);
}

ServletDispatch::ServletDispatch() : Servlet("ServletDispatch") {
    m_default.reset(new NotFoundServlet);
}

int32_t ServletDispatch::handle(HttpRequest::ptr request,
                                HttpResponse::ptr response,
                                HttpSession::ptr session) {
    auto slt = matched_servlet(request->path());
    if (slt) {
        slt->handle(request, response, session);
    }
    return 0;
}

void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt) {
    rwmutex_type::write_lock lock(m_mutex);
    m_datas[uri] = slt;
}

void ServletDispatch::addServlet(const std::string& uri,
                                 FunctionServlet::callback cb) {
    rwmutex_type::write_lock lock(m_mutex);
    m_datas[uri].reset(new FunctionServlet(cb));
}

void ServletDispatch::addGlobServlet(const std::string& uri, Servlet::ptr slt) {
    rwmutex_type::write_lock lock(m_mutex);
    for (auto p = m_globs.begin(); p != m_globs.end(); ++p) {
        if (p->first == uri) {
            m_globs.erase(p);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, slt));
}

void ServletDispatch::addGlobServlet(const std::string& uri,
                                     FunctionServlet::callback cb) {
    addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispatch::delServlet(const std::string& uri) {
    rwmutex_type::write_lock lock(m_mutex);
    m_datas.erase(uri);
}

void ServletDispatch::delGlobServlet(const std::string& uri) {
    rwmutex_type::write_lock lock(m_mutex);
    for (auto p = m_globs.begin(); p != m_globs.end(); ++p) {
        if (p->first == uri) {
            m_globs.erase(p);
            break;
        }
    }
}

Servlet::ptr ServletDispatch::servlet(const std::string& uri) {
    rwmutex_type::read_lock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatch::glob_servlet(const std::string& uri) {
    rwmutex_type::read_lock lock(m_mutex);
    for (auto p = m_globs.begin(); p != m_globs.end(); ++p) {
        if (p->first == uri) {
            return p->second;
        }
    }

    return nullptr;
}

Servlet::ptr ServletDispatch::matched_servlet(const std::string& uri) {
    rwmutex_type::read_lock lock(m_mutex);
    auto it = m_datas.find(uri);
    if (it != m_datas.end()) {
        return it->second;
    }
    for (auto p = m_globs.begin(); p != m_globs.end(); ++p) {
        if (!(::fnmatch(p->first.c_str(), uri.c_str(), 0))) {
            return p->second;
        }
    }
    return m_default;
}

NotFoundServlet::NotFoundServlet() : Servlet("NotFoundServlet") {}

int32_t NotFoundServlet::handle(HttpRequest::ptr request,
                                HttpResponse::ptr response,
                                HttpSession::ptr session) {
    static const std::string& RSP_BODY =
        "<html><head><title>404 NOT FOUND</title></head><body><center>404 NOT "
        "FOUND</center><hr><center>tihi/1.0.0</center></body></html>";
    response->set_status(tihi::http::http_status::HTTP_STATUS_NOT_FOUND);
    response->set_header("Content-Type", "text/html");
    response->set_body(RSP_BODY);
    return 0;
}

}  // namespace http
}  // namespace tihi