#include "socket.h"

#include "log/log.h"
#include "netinet/tcp.h"
#include "utils/macro.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

Socket::ptr Socket::TCPSocket() {
    Socket::ptr ret(new Socket(IPv4, TCP, 0));
    return ret;
}

Socket::ptr Socket::UDPSocket() {
    Socket::ptr ret(new Socket(IPv4, UDP, 0));
    return ret;
}

Socket::ptr Socket::TCPSocket6() {
    Socket::ptr ret(new Socket(IPv6, TCP, 0));
    return ret;
}

Socket::ptr Socket::UDPSocket6() {
    Socket::ptr ret(new Socket(IPv6, UDP, 0));
    return ret;
}

Socket::ptr Socket::TCPSocket(const Address::ptr addr) {
    Socket::ptr ret(new Socket(addr->family(), TCP, 0));
    return ret;
}

Socket::ptr Socket::UDPSocket(const Address::ptr addr) {
    Socket::ptr ret(new Socket(addr->family(), UDP, 0));
    return ret;
}

Socket::ptr Socket::UnixTCPSocket() {
    Socket::ptr ret(new Socket(UNIX, TCP, 0));
    return ret;
}

Socket::ptr Socket::UnixUDPSocket() {
    Socket::ptr ret(new Socket(UNIX, UDP, 0));
    return ret;
}

Socket::Socket(int family, int type, int protocol)
    : sockfd_(-1),
      family_(family),
      type_(type),
      protocol_(protocol),
      is_connected_(false) {}

Socket::~Socket() { close(); }

uint64_t Socket::send_timeout() const {
    FdCtx::ptr ctx = FdMgr::GetInstance()->fd(sockfd());
    if (ctx) {
        return ctx->timeout(SO_SNDTIMEO);
    }

    return -1;
}

void Socket::set_send_timeout(uint64_t timeout) {
    timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
    set_option(SOL_SOCKET, SO_SNDTIMEO, tv);
}

uint64_t Socket::recv_timeout() const {
    FdCtx::ptr ctx = FdMgr::GetInstance()->fd(sockfd());
    if (ctx) {
        return ctx->timeout(SO_RCVTIMEO);
    }

    return -1;
}

void Socket::set_recv_timeout(uint64_t timeout) {
    timeval tv{int(timeout / 1000), int(timeout % 1000 * 1000)};
    set_option(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::option(int level, int op, void* res, size_t* len) {
    int rt = getsockopt(sockfd_, level, op, res, (socklen_t*)len);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "getsockopt(sockfd=" << sockfd_ << ", level=" << level
            << ", op=" << op << ") errno=" << errno
            << " strerror=" << strerror(errno);
        return false;
    }

    return true;
}

bool Socket::set_option(int level, int op, void* val, long unsigned int len) {
    int rt = setsockopt(sockfd_, level, op, val, (socklen_t)len);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "setsockopt(sockfd=" << sockfd_ << ", level=" << level
            << ", op=" << op << ") errno=" << errno
            << " strerror=" << strerror(errno);
        return false;
    }

    return true;
}

bool Socket::init(int sockfd) {
    FdCtx::ptr ctx = FdMgr::GetInstance()->fd(sockfd);
    if (ctx && ctx->is_socket()) {
        sockfd = sockfd_;
        is_connected_ = true;
        local_addr();
        remote_addr();
        initSock();
        return true;
    }

    return false;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout) {
    if (!isValid()) {
        newSock();
        if (TIHI_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if (TIHI_UNLIKELY(family_ != addr->family())) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "family_=" << family_ << "addr->family()=" << addr->family()
            << " errno=" << errno << " strerror=" << strerror(errno);
    }

    if (timeout != ~0ul) {
        if (::connect(sockfd_, addr->addr(), addr->addrLen())) {
            TIHI_LOG_ERROR(g_sys_logger)
                << "connect(sockfd=" << sockfd_ << ", addr=" << addr->toString()
                << ", timeout=" << timeout << " errno=" << errno
                << " strerror=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if (tihi::connect_with_timeout(sockfd_, addr->addr(), addr->addrLen(),
                                       timeout)) {
            TIHI_LOG_ERROR(g_sys_logger)
                << "connect_with_timeout(sockfd=" << sockfd_
                << ", addr=" << addr->toString() << ", timeout=" << timeout
                << " errno=" << errno << " strerror=" << strerror(errno);
            close();
            return false;
        }
    }

    is_connected_ = true;
    remote_addr();
    local_addr();
    return true;
}

bool Socket::bind(const Address::ptr addr) {
    if (TIHI_UNLIKELY(!isValid())) {
        newSock();
        if (TIHI_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if (TIHI_UNLIKELY(family_ != addr->family())) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "family_=" << family_ << "addr->family()=" << addr->family()
            << " errno=" << errno << " strerror=" << strerror(errno);
    }

    if (::bind(sockfd_, addr->addr(), addr->addrLen())) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "bind(sockfd=" << sockfd_ << ", addr=" << addr->toString()
            << " errno=" << errno << " sterror=" << strerror(errno);
        return false;
    }

    local_addr();
    return true;
}

bool Socket::listen(int backlog) {
    if (!isValid()) {
        TIHI_LOG_ERROR(g_sys_logger) << "listen";
        return false;
    }

    if (::listen(sockfd_, backlog)) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "listen(sockfd_=" << sockfd_ << ", backlog=" << backlog
            << ") errno=" << errno << " strerror=" << strerror(errno);
        return false;
    }

    return true;
}

Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(family_, type_, protocol_));
    int connfd = ::accept(sockfd_, NULL, NULL);
    if (connfd) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "accept(fd=" << sockfd_ << ") errno=" << errno
            << " strerror=" << strerror(errno);
    }

    if (sock->init(connfd)) {
        return sock;
    }

    return nullptr;
}

bool Socket::close() {
    if (!is_connected_ && sockfd_ == -1) {
        return true;
    }

    is_connected_ = false;
    if (sockfd_ != -1) {
        ::close(sockfd_);
        sockfd_ = -1;
    }

    return true;
}

int Socket::send(const void* buf, size_t len, int flag) {
    if (isConnected()) {
        return ::send(sockfd_, buf, len, flag);
    }

    return -1;
}

int Socket::send(const iovec* buf, size_t sz, int flag) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buf;
        msg.msg_iovlen = sz;
        return ::sendmsg(sockfd_, &msg, flag);
    }

    return -1;
}

int Socket::sendto(const void* buf, size_t len, const Address::ptr to,
                   int flag) {
    if (isConnected()) {
        return ::sendto(sockfd_, buf, len, flag, to->addr(), to->addrLen());
    }
    return -1;
}

int Socket::sendto(const iovec* buf, size_t sz, const Address::ptr to,
                   int flag) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buf;
        msg.msg_iovlen = sz;
        msg.msg_name = to->addr();
        msg.msg_namelen = to->addrLen();
        return ::sendmsg(sockfd_, &msg, flag);
    }

    return -1;
}

int Socket::recv(void* buf, size_t len, int flag) {
    if (isConnected()) {
        return ::recv(sockfd_, buf, len, flag);
    }

    return -1;
}

int Socket::recv(iovec* buf, size_t sz, int flag) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buf;
        msg.msg_iovlen = sz;
        return ::recvmsg(sockfd_, &msg, flag);
    }

    return -1;
}

int Socket::recvfrom(void* buf, size_t len, Address::ptr from, int flag) {
    if (isConnected()) {
        socklen_t addr_len = from->addrLen();
        return ::recvfrom(sockfd_, buf, len, flag, from->addr(), &addr_len);
    }
    return -1;
}

int Socket::recvfrom(iovec* buf, size_t sz, Address::ptr from, int flag) {
    if (isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buf;
        msg.msg_iovlen = sz;
        msg.msg_name = from->addr();
        msg.msg_namelen = from->addrLen();
        return ::recvmsg(sockfd_, &msg, flag);
    }

    return -1;
}

Address::ptr Socket::remote_addr() {
    if (remote_addr_) {
        return remote_addr_;
    }

    Address::ptr res;
    switch (family_) {
        case AF_INET: {
            res.reset(new IPv4Address());
            break;
        }
        case AF_INET6: {
            res.reset(new IPv6Address());
            break;
        }
        case AF_UNIX: {
            res.reset(new UnixAddress());
            break;
        }
        default: {
            res.reset(new UnknownAddress(family_));
            break;
        }
    }

    socklen_t addr_len = res->addrLen();
    if (::getpeername(sockfd_, res->addr(), &addr_len)) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "getpeername(sockfd_=" << sockfd_ << ", addr=" << res->toString()
            << ") errno=" << errno << " strerror=" << strerror(errno);
        return Address::ptr(new UnknownAddress(family_));
    }

    if (AF_UNIX == family_) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(res);
        addr->set_len(addr_len);
    }

    remote_addr_ = res;
    return remote_addr_;
}

Address::ptr Socket::local_addr() {
    if (local_addr_) {
        return remote_addr_;
    }

    Address::ptr res;
    switch (family_) {
        case AF_INET: {
            res.reset(new IPv4Address());
            break;
        }
        case AF_INET6: {
            res.reset(new IPv6Address());
            break;
        }
        case AF_UNIX: {
            res.reset(new UnixAddress());
            break;
        }
        default: {
            res.reset(new UnknownAddress(family_));
            break;
        }
    }

    socklen_t addr_len = res->addrLen();
    if (::getsockname(sockfd_, res->addr(), &addr_len)) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "getsockname(sockfd_=" << sockfd_ << ", addr=" << res->toString()
            << ") errno=" << errno << " strerror=" << strerror(errno);
        return UnknownAddress::ptr(new UnknownAddress(family_));
    }

    if (AF_UNIX == family_) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(res);
        addr->set_len(addr_len);
    }

    local_addr_ = res;
    return local_addr_;
}

bool Socket::isValid() const {
    return sockfd_ != -1;
}

int Socket::error() {
    int error = 0;
    size_t len = sizeof(error);
    int rt = option(SOL_SOCKET, SO_ERROR, &error, &len);
    if (rt) {
        return rt;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
    os << "{Socket sockfd: " << sockfd_ << " family: " << family_
       << " type: " << type_ << " protocol: " << protocol_
       << " isconnected: " << isConnected() << " localAddr: "
       << ((local_addr_ != nullptr) ? local_addr_->toString() : 0)
       << " remoteAddr: "
       << ((remote_addr_ != nullptr) ? remote_addr_->toString() : 0);

    return os;
}

bool Socket::cancelAccept() {
    return IOManager::This()->cancelEvent(sockfd(), IOManager::READ);
}

bool Socket::cancelRead() {
    return IOManager::This()->cancelEvent(sockfd(), IOManager::READ);
}

bool Socket::cancelWrite() {
    return IOManager::This()->cancelEvent(sockfd(), IOManager::WRITE);
}

bool Socket::cancelAll() { return IOManager::This()->cancelAll(sockfd()); }

void Socket::initSock() {
    int v = 1;
    set_option(SOL_SOCKET, SO_REUSEADDR, v);
    if (type_ == SOCK_STREAM) {
        set_option(IPPROTO_TCP, TCP_NODELAY, v);
    }
}

void Socket::newSock() {
    sockfd_ = ::socket(family_, type_, protocol_);
    if (TIHI_LIKELY(sockfd_ != -1)) {
        initSock();
    } else {
        TIHI_LOG_ERROR(g_sys_logger)
            << "socket(family=" << family_ << ", type=" << type_
            << ", protocol=" << protocol_ << ") errno=" << errno
            << " strerror=" << strerror(errno);
    }
}
}  // namespace tihi