#ifndef TIHI_TCP_SERVER_TCP_SERVER_H_
#define TIHI_TCP_SERVER_TCP_SERVER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "iomanager/iomanager.h"
#include "socket/address/address.h"
#include "socket/socket/socket.h"
#include "utils/noncopyable.h"

namespace tihi {

class TcpServer : public std::enable_shared_from_this<TcpServer>,
                  public Noncopyable {
public:
    using ptr = std::shared_ptr<TcpServer>;
    TcpServer(IOManager* worker = IOManager::This(),
              IOManager* accept_worker = IOManager::This());
    virtual ~TcpServer();

    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs,
                      std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual void stop();

    uint64_t read_timeout() const { return read_timeout_; }
    void set_read_timeout(uint64_t read_timeout) {
        read_timeout_ = read_timeout;
    }
    const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    bool is_stop() const { return is_stop_; }

protected:
    virtual void handleClient(Socket::ptr sock);
    virtual void startAccepet(Socket::ptr sock);

private:
    std::vector<Socket::ptr> socks_;
    IOManager* worker_;
    IOManager* accept_worker_;
    uint64_t read_timeout_;
    std::string name_;
    bool is_stop_;
};

}  // namespace tihi

#endif  // TIHI_TCP_SERVER_TCP_SERVER_H_