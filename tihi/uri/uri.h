#ifndef TIHI_URI_URI_H_
#define TIHI_URI_URI_H_

#include <string>
#include <memory>
#include <stdint.h>

#include "socket/address/address.h"

namespace tihi {

/*
   The following are two example URIs and their component parts:

         foo://example.com:8042/over/there?name=ferret#nose
         \_/   \______________/\_________/ \_________/ \__/
          |           |            |            |        |
       scheme     authority       path        query   fragment
          |   _____________________|__
         / \ /                        \
         urn:example:animal:ferret:nose
*/
class Uri {
public:
    using ptr = std::shared_ptr<Uri>;
    Uri();

    const std::string& scheme() const { return m_scheme; }
    const std::string& userinfo() const { return m_userinfo; }
    const std::string& host() const { return m_host; }
    const std::string& path() const;
    const std::string& query() const { return m_query; }
    const std::string& fragment() const { return m_fragment; }
    int32_t port() const;

    void set_scheme(const std::string& v) { m_scheme = v; }
    void set_userinfo(const std::string& v) { m_userinfo = v; }
    void set_host(const std::string& v) { m_host = v; }
    void set_path(const std::string& v) { m_path = v; }
    void set_query(const std::string& v) { m_query = v; }
    void set_fragment(const std::string& v) { m_fragment = v; }
    void set_port(int32_t port) { m_port = port; }

    static Uri::ptr Create(const std::string& uri);
    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    Address::ptr createAddress() const;
private:
    bool isDefaultPort() const;

private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    uint32_t m_port;
};

} // namespace tihi

#endif // TIHI_URI_URI_H_