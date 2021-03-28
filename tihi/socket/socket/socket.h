#ifndef TIHI_SOCKET_SOCKET_SOCKET_H_
#define TIHI_SOCKET_SOCKET_SOCKET_H_

#include <memory>

#include "socket/address/address.h"
#include "utils/noncopyable.h"
#include "hook/fd_manager.h"
#include "hook/hook.h"

namespace tihi {

class Socket : public Noncopyable, public std::enable_shared_from_this<Socket> {
public:
    using ptr = std::shared_ptr<Socket>;
    using weak_ptr = std::weak_ptr<Socket>;

    enum Family {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX,
    };

    enum Type {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
    };

    Socket(int family, int type, int protocol = 0);
    ~Socket();

    uint64_t send_timeout() const;
    void set_send_timeout(uint64_t timeout);

    uint64_t recv_timeout() const;
    void set_recv_timeout(uint64_t timeout);

    bool option(int level, int op, void* res, size_t* len);
    template <typename T>
    bool option(int level, int op, T& res) {
        size_t len = sizeof(T);
        return option((level, op, &res, &len));
    }

    bool set_option(int level, int op, void* val, long unsigned int len);
    template <typename T>
    bool set_option(int level, int op, const T& val) {
        return set_option(level, op, (void*)&val, sizeof(T));
    }

    bool connect(const Address::ptr addr, uint64_t timeout = ~0ul);

    bool bind(const Address::ptr addr);
    bool listen(int backlog = SOMAXCONN);
    Socket::ptr accept();
    bool close();

    int send(const void* buf, size_t len, int flag = 0);
    int send(const iovec* buf, size_t sz, int flag = 0);
    int sendto(const void* buf, size_t len, const Address::ptr to, int flag = 0);
    int sendto(const iovec* buf, size_t sz, const Address::ptr to, int flag = 0);

    int recv(void* buf, size_t len, int flag = 0);
    int recv(iovec* buf, size_t sz, int flag = 0);
    int recvfrom(void* buf, size_t len, Address::ptr from, int flag);
    int recvfrom(iovec* buf, size_t sz, Address::ptr from, int flag);

    Address::ptr remote_addr();
    Address::ptr local_addr();

    int family() const { return family_; }
    int type() const { return type_; }
    int protocol() const { return protocol_; }

    bool isConnected() const { return is_connected_; }
    bool isValid() const;
    int error();

    std::ostream& dump(std::ostream& os) const;
    int sockfd() const { return sockfd_; }

    bool cancelAccept();
    bool cancelRead();
    bool cancelWrite();
    bool cancelAll();

    static ptr TCPSocket();
    static ptr UDPSocket();

    static ptr TCPSocket6();
    static ptr UDPSocket6();

    static ptr TCPSocket(const Address::ptr addr);
    static ptr UDPSocket(const Address::ptr addr);

    static ptr UnixTCPSocket();
    static ptr UnixUDPSocket();

private:
    void initSock();
    void newSock();
    bool init(int sockfd);

private:
    int sockfd_ = -1;
    int family_ = 0;
    int type_ = 0;
    int protocol_ = 0;
    bool is_connected_ = false;
    
    Address::ptr local_addr_;
    Address::ptr remote_addr_;
};

std::ostream& operator<<(std::ostream& os, const Socket& sock);

} // namespace tihi

#endif // TIHI_SOCKET_SOCKET_SOCKET_H_
