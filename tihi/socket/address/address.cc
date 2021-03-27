#include "address.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <sstream>

#include "log/log.h"
#include "utils/endian.h"
#include "utils/macro.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

/**
 * 创建类型为 T 的 低 bits 位为 1 ，高位全为 0 的掩码
 * 例如：T 为 uint16_t，bits 为 4 则此函数返回：
 * 0000 0000 0000 1111
 */
template <typename T>
T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

Address::ptr Address::LookUpAny(const std::string& host, int family, int type,
                                int protocol) {
    std::vector<Address::ptr> res;
    int rt = LookUp(res, host, family, type, protocol);
    if (rt) {
        return res[0];
    }

    return nullptr;
}

Address::ptr Address::LookUpIPAddress(const std::string& host, int family,
                                      int type, int protocol) {
    std::vector<Address::ptr> res;
    int rt = LookUp(res, host, family, type, protocol);
    if (rt) {
        IPAddress::ptr ans = nullptr;
        for (auto a : res) {
            ans = std::dynamic_pointer_cast<IPAddress>(a);
            if (ans) {
                return ans;
            }
        }
    }

    return nullptr;
}

bool Address::LookUp(std::vector<Address::ptr>& res, const std::string& host,
                     int family, int type, int protocol) {
    struct addrinfo hints;
    struct addrinfo *results, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    std::string ip;
    const char* service = nullptr;

    if (!host.empty() && host[0] == '[') {
        const char* ip_end =
            (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if (ip_end) {
            if (*(ip_end + 1) == ':') {
                service = ip_end + 2;
            }
            ip = host.substr(1, ip_end - host.c_str() - 1);
        }
    }

    if (ip.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if (service) {
            if (!memchr(service + 1, ':',
                        host.c_str() + host.size() - service - 1)) {
                ip = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    if (ip.empty()) {
        ip = host;
    }

    int rt = getaddrinfo(ip.c_str(), service, &hints, &results);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "getaddrinfo(ip=" << ip << ", service=" << service
            << ") errno=" << errno << " strerror: " << strerror(errno);
        return false;
    }

    if (!results) {
        return false;
    }

    rp = results;
    while (rp) {
        res.push_back(Create(rp->ai_addr, rp->ai_addrlen));
        rp = rp->ai_next;
    }

    freeaddrinfo(results);

    return true;
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t len) {
    if (!addr) {
        return nullptr;
    }

    Address::ptr res;
    switch (addr->sa_family) {
        case AF_INET: {
            res.reset(new IPv4Address(*(const sockaddr_in*)addr));
            break;
        }
        case AF_INET6: {
            res.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        }
        default: {
            res.reset(new UnknownAddress(*(addr)));
            break;
        }
    }

    return res;
}


IPAddress::ptr IPAddress::Create(const char* addr, uint32_t port) {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    struct addrinfo* results;
    int s = getaddrinfo(addr, std::to_string(port).c_str(), &hints, &results);
    if (s) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "s=" << s << " getaddinfo(addr=" << addr << ", port=" << port
            << "...) errno=" << errno << " strerror=" << strerror(errno);
        return nullptr;
    }

    try {
        IPAddress::ptr res = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, results->ai_addrlen));
        if (res) {
            res->set_port(port);
        }
        freeaddrinfo(results);
        return res;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }

    return nullptr;
}


int Address::family() const { return addr()->sa_family; }

std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const {
    uint32_t min_len = std::min(addrLen(), rhs.addrLen());
    int result = memcmp(addr(), rhs.addr(), min_len);
    if (result < 0) {
        return true;
    } else if (result > 0) {
        return false;
    } else if (addrLen() < rhs.addrLen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address& rhs) const {
    return (addrLen() == rhs.addrLen() &&
            memcmp(addr(), rhs.addr(), addrLen()) == 0);
}

bool Address::operator!=(const Address& rhs) const { return !(*this == rhs); }

IPv4Address::IPv4Address(const sockaddr_in& addr) { addr_ = addr; }

IPv4Address::ptr IPv4Address::Create(const char* ip, uint32_t port) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = hton(port);
    int rt = inet_pton(AF_INET, ip, &(addr.sin_addr));
    if (!rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "rt=" << rt << " inet_aton(" << ip << ", "
            << addr.sin_addr.s_addr << ") errno=" << errno
            << " strerror: " << strerror(errno);
    }

    return IPv4Address::ptr(new IPv4Address(addr));
}


IPv4Address::IPv4Address(uint32_t addr, uint32_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = hton(port);
    addr_.sin_addr.s_addr = hton(addr);
}

const sockaddr* IPv4Address::addr() const { return (sockaddr*)(&addr_); }

socklen_t IPv4Address::addrLen() const { return sizeof(addr_); }

std::ostream& IPv4Address::insert(std::ostream& os) const {
    uint32_t addr_h = ntoh(addr_.sin_addr.s_addr);

    os << ((addr_h >> 24) & 0xff) << "." << ((addr_h >> 16) & 0xff) << "."
       << ((addr_h >> 8) & 0xff) << "." << (addr_h & 0xff);
    os << ":" << ntoh(addr_.sin_port);

    return os;
}

IPAddress::ptr IPv4Address::broadcastAddr(uint32_t prefix_len) const {
    if (prefix_len > addrLen()) {
        return nullptr;
    }

    sockaddr_in baddr(addr_);
    baddr.sin_addr.s_addr |= hton(CreateMask<uint32_t>(32 - prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networkAddr(uint32_t prefix_len) const {
    if (prefix_len > addrLen()) {
        return nullptr;
    }

    sockaddr_in addr(addr_);
    addr.sin_addr.s_addr &= hton(CreateMask<uint32_t>(~prefix_len));
    return IPv4Address::ptr(new IPv4Address(addr));
}

uint32_t IPv4Address::subnetMask(uint32_t prefix_len) const {
    return (hton(CreateMask<uint32_t>(~prefix_len)));
}

uint16_t IPv4Address::port() const { return ntoh(addr_.sin_port); }

void IPv4Address::set_port(uint16_t port) {
    addr_.sin_port = hton(port);
    // TIHI_LOG_INFO(g_sys_logger)
    //     << addr_.sin_port << "  " << port << " hton(port): " << hton(port)
    //     << "  " << htons(port);
}

IPv6Address::IPv6Address() {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& addr) { addr_ = addr; }

IPv6Address::ptr IPv6Address::Create(const char* ip, uint32_t port) {
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET;
    addr.sin6_port = hton(port);
    int rt = inet_pton(AF_INET6, ip, &(addr.sin6_addr));
    if (!rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "rt=" << rt << " inet_aton(" << ip << ", "
            << addr.sin6_addr.s6_addr << ") errno=" << errno
            << " strerror: " << strerror(errno);
    }

    return IPv6Address::ptr(new IPv6Address(addr));
}

IPv6Address::IPv6Address(const uint8_t addr[16], uint32_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin6_family = AF_INET6;
    addr_.sin6_port = hton(port);
    memcpy(&addr_.sin6_addr.s6_addr, addr, 16);
}

const sockaddr* IPv6Address::addr() const { return (sockaddr*)(&addr_); }

socklen_t IPv6Address::addrLen() const { return sizeof(addr_); }

/** 0位压缩表示法
 *  FF01:0:0:0:0:0:0:1101 → FF01::1101
 *  0:0:0:0:0:0:0:1 → ::1
 *  0:0:0:0:0:0:0:0 → ::
 */
std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)addr_.sin6_addr.s6_addr;
    bool used_zero = false;
    for (int i = 0; i < 8; ++i) {
        if (!addr[i] && !used_zero) {
            continue;
        }
        if (i && !addr[i - 1] && !used_zero) {
            os << ":";
            used_zero = true;
        }
        if (i) {
            os << ":";
        }
        os << std::hex << ntoh(addr[i]) << std::dec;
    }

    if (!used_zero && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << ntoh(addr_.sin6_port);

    return os;
}

IPAddress::ptr IPv6Address::broadcastAddr(uint32_t prefix_len) const {
    sockaddr_in6 baddr(addr_);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);  // 隐约觉得有问题
    for (int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }

    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networkAddr(uint32_t prefix_len) const {
    sockaddr_in6 addr(addr_);
    addr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);  // 隐约觉得有问题

    return IPv6Address::ptr(new IPv6Address(addr));
}

uint32_t IPv6Address::subnetMask(uint32_t prefix_len) const {
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr.s6_addr[prefix_len / 8] =
        ~CreateMask<uint8_t>(prefix_len % 8);  // 隐约觉得有问题

    for (size_t i = 0; i < prefix_len / 8; ++i) {
        addr.sin6_addr.s6_addr[i] = 0xff;
    }

    // return IPv6Address::ptr(new IPv6Address(addr));

    return 0;
}

uint16_t IPv6Address::port() const { return ntoh(addr_.sin6_port); }

void IPv6Address::set_port(uint16_t port) { addr_.sin6_port = hton(port); }


/**
 * 107
 */
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sun_family = AF_UNIX;
    len_ = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sun_family = AF_UNIX;
    len_ = path.size() + 1;

    if (!path.empty() && path[0] == '\0') {
        --len_;
    }

    if (len_ > sizeof(addr_.sun_path)) {
        throw std::logic_error("unix path too long");
    }

    memcpy(addr_.sun_path, path.c_str(), len_);
    len_ += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::addr() const { return (sockaddr*)(&addr_); }

socklen_t UnixAddress::addrLen() const { return len_; }

std::ostream& UnixAddress::insert(std::ostream& os) const {
    if (len_ > offsetof(sockaddr_un, sun_path) && addr_.sun_path[0] == '\0') {
        return os << "\\0"
                  << std::string(addr_.sun_path + 1,
                                 len_ - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << addr_.sun_path;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) { addr_ = addr; }

UnknownAddress::UnknownAddress(int family) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sa_family = family;
}

const sockaddr* UnknownAddress::addr() const { return (sockaddr*)(&addr_); }

socklen_t UnknownAddress::addrLen() const { return sizeof(addr_); }

std::ostream& UnknownAddress::insert(std::ostream& os) const {
    return os << "[UNKNOWN_ADDRESS family: " << addr_.sa_family << "]";
}

}  // namespace tihi