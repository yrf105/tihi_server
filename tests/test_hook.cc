#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hook/hook.h"
#include "iomanager/iomanager.h"
#include "log/log.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void test_sleep() {
    // tihi::set_hook_enable(true);
    tihi::IOManager iom(1);
    iom.schedule([]() {
        sleep(2);
        TIHI_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([]() {
        sleep(3);
        TIHI_LOG_INFO(g_logger) << "sleep 4";
    });

    TIHI_LOG_INFO(g_logger) << "test_hook";
}

void test_socket() {

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton("39.156.69.79", &(server_addr.sin_addr));
    server_addr.sin_port = htons(80);

    int rt =
        connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    TIHI_LOG_DEBUG(g_logger) << "sockfd: " << sockfd << " connect rt = " << rt << " errno: " << errno
                             << " strerror: " << strerror(errno);
    
    if (rt) {
        close(sockfd);
        return ;
    }

    const char http[] = "GET / HTTP/1.0\r\n\r\n";
    char buffer[4096];
    rt = send(sockfd, http, strlen(http), 0);

    rt = recv(sockfd, buffer, 4095, 0);
    buffer[rt] = '\0';
    TIHI_LOG_DEBUG(g_logger) << buffer;
    close(sockfd);
}

void test_log() {
    TIHI_LOG_DEBUG(g_logger) << "hello";
}

int main(int argc, char** argv) {
    // test_sleep();
    tihi::IOManager iom(1, true);
    iom.schedule(&test_socket);
    return 0;
}