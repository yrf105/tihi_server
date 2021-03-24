#ifndef TIHI_SOCKET_ADDRESS_ADDRESS_H_
#define TIHI_SOCKET_ADDRESS_ADDRESS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include <memory>

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
private:
};

class IPAddress : public Address {
public:
    using ptr = std::shared_ptr<IPAddress>;
    virtual ~IPAddress() {}
    virtual IPAddress::ptr broadcastAddr(uint32_t prefix_len) const = 0;
    virtual IPAddress::ptr networkAddr(uint32_t prefix_len) const = 0;
    virtual uint32_t subnetMask(uint32_t prefix_len) const = 0;
    virtual uint32_t port() const = 0;
    virtual void set_port(uint32_t port) = 0;
private:
};

class IPv4Address : public IPAddress {
public:
    using ptr = std::shared_ptr<IPAddress>;
    IPv4Address(uint32_t addr = INADDR_ANY, uint32_t port = 0);
    const sockaddr* addr() const override;
    socklen_t addrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
    IPAddress::ptr broadcastAddr(uint32_t prefix_len) const override;
    IPAddress::ptr networkAddr(uint32_t prefix_len) const override;
    uint32_t subnetMask(uint32_t prefix_len) const override;
    uint32_t port() const override;
    void set_port(uint32_t port) override;

private:
    sockaddr_in addr_;
};

class IPv6Address : public IPAddress {
public:
    using ptr = std::shared_ptr<IPv6Address>;
    
    IPv6Address(uint32_t addr = INADDR_ANY, uint32_t port = 0);

    const sockaddr* addr() const override;
    socklen_t addrLen() const override;
    std::ostream& insert(std::ostream& os) const override;
    IPAddress::ptr broadcastAddr(uint32_t prefix_len) const override;
    IPAddress::ptr networkAddr(uint32_t prefix_len) const override;
    uint32_t subnetMask(uint32_t prefix_len) const override;
    uint32_t port() const override;
    void set_port(uint32_t port) override;
private:
    sockaddr_in6 addr_;
};

class UnixAddress : public Address {
public:
    using ptr = std::shared_ptr<UnixAddress>;
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
    UnknownAddress();

    const sockaddr* addr() const override;
    socklen_t addrLen() const override;
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr addr_;
};

} // namespace tihi

#endif // TIHI_SOCKET_ADDRESS_ADDRESS_H_