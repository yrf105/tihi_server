#include "socket/socket/socket.h"
#include "log/log.h"
#include "iomanager/iomanager.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void test_socket() {
    TIHI_LOG_INFO(g_logger) << "start";
    tihi::Address::ptr addr = tihi::IPAddress::LookUpAny("www.baidu.com:80");
    if (addr) {
        TIHI_LOG_INFO(g_logger) << addr->toString();
    } else {
        TIHI_LOG_ERROR(g_logger) << "errno: " << errno << " strerror: " << strerror(errno);
        return ;
    }

    tihi::Socket::ptr sock = tihi::Socket::TCPSocket(addr);
    if (!sock) {
        TIHI_LOG_ERROR(g_logger) << "errno: " << errno << " strerror: " << strerror(errno);
        return ;
    }

    int rt = sock->connect(addr);
    if (!rt) {
        TIHI_LOG_ERROR(g_logger) << "rt: " << rt << " errno: " << errno << " strerror: " << strerror(errno);
        return ;
    }

    const char buf[] = "GET / HTTP/1.0\r\n\r\n";
    std::string buff;
    buff.resize(4096);
    rt = sock->send(buf, sizeof(buf));
    if (!rt) {
        TIHI_LOG_ERROR(g_logger) << "rt: " << rt << " errno: " << errno << " strerror: " << strerror(errno);
        return ;
    }
    rt = sock->recv((&(buff[0])), 4096);
    if (rt >= 0) {
        TIHI_LOG_INFO(g_logger) << buff;
    } else {
        TIHI_LOG_ERROR(g_logger) << "rt: " << rt << " errno: " << errno << " strerror: " << strerror(errno);
        return ;
    }
}

int main(int argc, char** argv) {
    tihi::IOManager iom;
    iom.schedule(&test_socket);
    
    return 0;
}