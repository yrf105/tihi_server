#include "tcp_server.h"

#include "config/config.h"
#include "log/log.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

static ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    Config::Lookup<uint64_t>("tcp_server.read_time", (uint64_t)(60 * 1000 * 2),
                             "tcp server read timeout");

TcpServer::TcpServer(IOManager* worker, IOManager* accept_worker)
    : worker_(worker),
      accept_worker_(accept_worker),
      read_timeout_(g_tcp_server_read_timeout->value()),
      name_("tihi/1.0.0"),
      is_stop_(true) {}

TcpServer::~TcpServer() {
    for (auto& sock : socks_) {
        sock->close();
    }
    socks_.clear();
}

bool TcpServer::bind(Address::ptr addr) {
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs,
                     std::vector<Address::ptr>& fails) {
    for (const auto& addr : addrs) {
        Socket::ptr sock = Socket::TCPSocket(addr);
        if (!sock->bind(addr)) {
            TIHI_LOG_WARN(g_sys_logger)
                << "bind(" << addr->toString() << ") errno=" << errno
                << " strerror=" << strerror(errno);
            fails.push_back(addr);
            continue;
        }
        if (!sock->listen()) {
            TIHI_LOG_WARN(g_sys_logger)
                << "listen(" << addr->toString() << ") errno=" << errno
                << " strerror=" << strerror(errno);
            fails.push_back(addr);
            continue;
        }
        socks_.push_back(sock);
    }

    if (!fails.empty()) {
        // socks_.clear(); // ?? 要清空吗？
        return false;
    }

    for (auto& sock : socks_) {
        TIHI_LOG_INFO(g_sys_logger) << "bind success: " << *(sock);
    }

    return true;
}

void TcpServer::startAccepet(Socket::ptr sock) {
    while (!is_stop_) {
        Socket::ptr client = sock->accept();
        if (client) {
            worker_->schedule(std::bind(&TcpServer::handleClient,
                                        shared_from_this(), client));
        } else {
            TIHI_LOG_ERROR(g_sys_logger)
                << "accept(" << *sock << ") errno=" << errno
                << " strerror=" << strerror(errno);
        }
    }
}

bool TcpServer::start() {
    if (!is_stop_) {
        return true;
    }
    is_stop_ = false;

    for (auto& sock : socks_) {
        accept_worker_->schedule(
            std::bind(&TcpServer::startAccepet, shared_from_this(), sock));
    }

    return true;
}

void TcpServer::stop() {
    is_stop_ = true;

    auto self = shared_from_this();
    accept_worker_->schedule([this, self](){
        for (auto& sock : socks_) {
            sock->cancelAll();
            sock->close();
        }
        socks_.clear();
    });
}

void TcpServer::handleClient(Socket::ptr sock) {
    TIHI_LOG_INFO(g_sys_logger) << *sock;
}

}  // namespace tihi