#include "tcp_server/tcp_server.h"
#include "iomanager/iomanager.h"
#include "log/log.h"

#include <memory>

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void run() {
    auto addr = tihi::Address::LookUpAny("0.0.0.0:8848");
    // auto uaddr = std::shared_ptr<tihi::UnixAddress>(new tihi::UnixAddress("/tmp/unix_addr"));
    TIHI_LOG_INFO(g_logger) << *addr;

    std::vector<tihi::Address::ptr> addrs;
    std::vector<tihi::Address::ptr> fails;
    addrs.push_back(addr);
    // addrs.push_back(uaddr);
    tihi::TcpServer::ptr tcp_server(new tihi::TcpServer);
    tcp_server->bind(addrs, fails);

    tcp_server->start();
}

int main(int argc, char** argv) {
    tihi::IOManager iom(1);
    iom.schedule(run);
    return 0;
}