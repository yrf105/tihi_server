#ifndef TIHI_SOCKET_ADDRESS_ADDRESS_H_
#define TIHI_SOCKET_ADDRESS_ADDRESS_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <memory>
#include <vector>
#include <map>

namespace tihi {

class Address {
public:
    using ptr = std::shared_ptr<Address>;
    virtual ~Address() {}
    int family() const;
    virtual const sockaddr* addr() const = 0;
    virtual socklen_t addrLen() const = 0;
    virtual std::ostream& insert(std::ostream& os) const = 0;
    std::string toString() const;
    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;

    static Address::ptr Create(const sockaddr* addr, socklen_t len);
    static bool LookUp(std::vector<Address::ptr>& res, const std::string& host,
                       int family = AF_UNSPEC, int type = 0, int protocol = 0);
    static Address::ptr LookUpAny(const std::string& host,
                                  int family = AF_UNSPEC, int type = 0,
                                  int protocol = 0);
    static Address::ptr LookUpIPAddress(const std::string& host,
                                        int family = AF_UNSPEC, int type = 0,
                                        int protocol = 0);
    /**
     * 获得网卡地址
     * 使用 getifaddrs 实现
     */
    static bool GetInterfaceAddress(
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& res,
        int family);
    static bool GetInterfaceAddress(
        std::vector<std::pair<Address::ptr, uint32_t>>& res,
        const std::string& ifa_name, int family);

private:
};

class IPAddress : public Address {
public:
    using ptr = std::shared_ptr<IPAddress>;
    virtual ~IPAddress() {}
    virtual IPAddress::ptr broadcastAddr(uint32_t prefix_len) const = 0;
    virtual IPAddress::ptr networkAddr(uint32_t prefix_len) const = 0;
    virtual uint32_t subnetMask(uint32_t prefix_len) const = 0;
    virtual uint16_t port() const = 0;
    virtual void set_port(uint16_t port) = 0;

    static IPAddress::ptr Create(const char* addr, uint32_t port);

private:
};

class IPv4Address : public IPAddress {
public:
    using ptr = std::shared_ptr<IPAddress>;
    IPv4Address(const sockaddr_in& addr);
    IPv4Address(uint32_t addr = INADDR_ANY, uint32_t port = 0);
    const sockaddr* addr() const override;
    socklen_t addrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
    IPAddress::ptr broadcastAddr(uint32_t prefix_len) const override;
    IPAddress::ptr networkAddr(uint32_t prefix_len) const override;
    uint32_t subnetMask(uint32_t prefix_len) const override;
    uint16_t port() const override;
    void set_port(uint16_t port) override;

    static IPv4Address::ptr Create(const char* ip, uint32_t port = 0);

private:
    sockaddr_in addr_;
};

class IPv6Address : public IPAddress {
public:
    using ptr = std::shared_ptr<IPv6Address>;

    IPv6Address();
    IPv6Address(const sockaddr_in6& addr);
    IPv6Address(const uint8_t addr[16], uint32_t port = 0);

    const sockaddr* addr() const override;
    socklen_t addrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
    IPAddress::ptr broadcastAddr(uint32_t prefix_len) const override;
    IPAddress::ptr networkAddr(uint32_t prefix_len) const override;
    uint32_t subnetMask(uint32_t prefix_len) const override;
    uint16_t port() const override;
    void set_port(uint16_t port) override;


    static IPv6Address::ptr Create(const char* ip, uint32_t port = 0);

private:
    sockaddr_in6 addr_;
};

class UnixAddress : public Address {
public:
    using ptr = std::shared_ptr<UnixAddress>;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* addr() const override;
    socklen_t addrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_un addr_;
    socklen_t len_;
};

class UnknownAddress : public Address {
public:
    using ptr = std::shared_ptr<UnknownAddress>;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);

    const sockaddr* addr() const override;
    socklen_t addrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr addr_;
};

}  // namespace tihi

#endif  // TIHI_SOCKET_ADDRESS_ADDRESS_H_