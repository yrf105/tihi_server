#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "iomanager/iomanager.h"
#include "log/log.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void f() {
    TIHI_LOG_INFO(g_logger) << "hello iomanager......";
}

void test() {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton("220.181.38.148", &(server_addr.sin_addr));
    server_addr.sin_port = htons(80);

    tihi::IOManager::This()->addEvent(sockfd, tihi::IOManager::WRITE, [](){
        TIHI_LOG_INFO(g_logger) << "hello baidu...";
    });

    connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
}

void my_test() {
    tihi::IOManager iom(2, false);
    iom.schedule(&test);
}

tihi::Timer::ptr timer;
void test_timer() {
    tihi::IOManager iom(1, false);
    timer = iom.addTimer(3000, [](){
        TIHI_LOG_INFO(g_logger) << "hello timer";
        timer->cancel();
    }, true);
}

int main(int argc, char** argv) {
    // my_test();
    test_timer();
    return 0;
}