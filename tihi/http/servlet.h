#ifndef TIHI_HTTP_SERVLET_H_
#define TIHI_HTTP_SERVLET_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "http.h"
#include "http_session.h"
#include "utils/mutex.h"

namespace tihi {
namespace http {
class Servlet {
public:
    using ptr = std::shared_ptr<Servlet>;

    Servlet(const std::string& name = "") : m_name(name) {}
    virtual ~Servlet() {}

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) = 0;
    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    using ptr = std::shared_ptr<FunctionServlet>;
    using callback = std::function<int32_t(HttpRequest::ptr request,
                                           HttpResponse::ptr response,
                                           HttpSession::ptr session)>;
    FunctionServlet(callback cb);

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) override;

private:
    callback m_cb;
};

class ServletDispatch : public Servlet {
public:
    using rwmutex_type = RWMutex;
    using ptr = std::shared_ptr<ServletDispatch>;

    ServletDispatch();

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr servlet(const std::string& uri);
    Servlet::ptr glob_servlet(const std::string& uri);

    Servlet::ptr matched_servlet(const std::string& uri);

    Servlet::ptr default_servlet() { return m_default; }
    void set_default_servlet(Servlet::ptr slt) { m_default = slt; }

private:
    // 精准命中
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // 模糊匹配
    std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
    // 默认
    Servlet::ptr m_default;

    rwmutex_type m_mutex;
};

class NotFoundServlet : public Servlet {
public:
    using ptr = std::shared_ptr<NotFoundServlet>;

    NotFoundServlet();

    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) override;
};
}  // namespace http
}  // namespace tihi

#endif  // TIHI_HTTP_SERVLET_H_