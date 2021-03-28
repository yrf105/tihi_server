#include "iomanager/iomanager.h"
#include "tcp_server/tcp_server.h"
#include "log/log.h"
#include "bytearray/bytearray.h"
#include <iostream>

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

class EchoServer : public tihi::TcpServer{
public:
    using ptr = std::shared_ptr<EchoServer>;
    EchoServer(int type);
    void handleClient(tihi::Socket::ptr sock) override;

private:
    int m_type;

};

EchoServer::EchoServer(int type) : m_type(type) {

}

// char buffer[2048];

void EchoServer::handleClient(tihi::Socket::ptr sock) {
    TIHI_LOG_INFO(g_logger) << "handleClient: " << *sock;
    tihi::ByteArray::ptr ba(new tihi::ByteArray);
    while (true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);
        int rt = sock->recv(&iovs[0], iovs.size());
        

        // int rt = sock->recv(buffer, 2047, 0);
        ba->set_postion(ba->postion() + rt);

        if (rt == 0) {
            TIHI_LOG_INFO(g_logger) << "client close: " << *sock;
            break;
        } else if (rt < 0) {
            TIHI_LOG_INFO(g_logger) << "errno=" << errno << " strerror=" << strerror(errno);
            break;
        }

        // buffer[rt] = '\0';
        // TIHI_LOG_DEBUG(g_logger) << buffer;

        ba->set_postion(0);
        if (m_type == 1) {
            TIHI_LOG_DEBUG(g_logger) << "echo: " << std::string((char*)(iovs[0].iov_base), rt);
            TIHI_LOG_INFO(g_logger) << "echo: " << ba->toString();
        } else {
            TIHI_LOG_INFO(g_logger) << "echo: " << ba->toHexString();
        }
    }
}

int type = 1;

void run() {
    EchoServer::ptr es(new EchoServer(type));
    tihi::Address::ptr addr = tihi::Address::LookUpAny("0.0.0.0:8848");
    while(!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argv, char** argc) {

    if (argv != 2) {
        std::cout << "Usage: " << argc[0] << " -b/-t\n";
        return -1;
    }

    if (!strcmp(argc[1], "-b")) {
        type = 2;
    }

    tihi::IOManager iom(1);
    iom.schedule(run);
    return 0;
}