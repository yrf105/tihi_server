#include "address.h"

#include <sstream>

namespace tihi {

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

bool Address::operator!=(const Address& rhs) const {
    return !(*this == rhs);
}

IPAddress::ptr IPAddress::broadcastAddr(uint32_t prefix_len) const {}
IPAddress::ptr IPAddress::networkAddr(uint32_t prefix_len) const {}
uint32_t IPAddress::subnetMask(uint32_t prefix_len) const {}
uint32_t IPAddress::port() const {}
void IPAddress::set_port(uint32_t port) {}

IPv4Address::IPv4Address(uint32_t addr = INADDR_ANY, uint32_t port = 0) {}
const sockaddr* IPv4Address::addr() const {}
socklen_t IPv4Address::addrLen() const {}
std::ostream& IPv4Address::insert(std::ostream& os) const {}
IPAddress::ptr IPv4Address::broadcastAddr(uint32_t prefix_len) const {}
IPAddress::ptr IPv4Address::networkAddr(uint32_t prefix_len) const {}
uint32_t IPv4Address::subnetMask(uint32_t prefix_len) const {}
uint32_t IPv4Address::port() const {}
void IPv4Address::set_port(uint32_t port) {}

IPv6Address::IPv6Address(uint32_t addr = INADDR_ANY, uint32_t port = 0) {}
const sockaddr* IPv6Address::addr() const {}
socklen_t IPv6Address::addrLen() const {}
std::ostream& IPv6Address::insert(std::ostream& os) const {}
IPAddress::ptr IPv6Address::broadcastAddr(uint32_t prefix_len) const {}
IPAddress::ptr IPv6Address::networkAddr(uint32_t prefix_len) const {}
uint32_t IPv6Address::subnetMask(uint32_t prefix_len) const {}
uint32_t IPv6Address::port() const {}
void IPv6Address::set_port(uint32_t port) {}

UnixAddress::UnixAddress(const std::string& path) {}
    
const sockaddr* UnixAddress::addr() const {}
socklen_t UnixAddress::addrLen() const {}
std::ostream& UnixAddress::insert(std::ostream& os) const {}

UnknownAddress::UnknownAddress() {}

const sockaddr* UnknownAddress::addr() const {}
socklen_t UnknownAddress::addrLen() const {}
std::ostream& UnknownAddress::insert(std::ostream& os) const {}

}  // namespace tihi