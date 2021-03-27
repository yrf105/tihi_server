#include "socket/address/address.h"
#include "log/log.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

static void test() {
    std::vector<tihi::Address::ptr> res;
    bool rt = tihi::Address::LookUp(res, "www.sylar.top");
    if (!rt) {
        TIHI_LOG_ERROR(g_logger) << "rt=" << rt;
    }

    for (auto a : res) {
        TIHI_LOG_INFO(g_logger) << a->toString();
    }
}

static void test_ipv4() {
    tihi::IPAddress::ptr addr = tihi::IPAddress::Create("www.baidu.com", 80);
    TIHI_LOG_INFO(g_logger) << addr->toString();
    addr->set_port(80);
    TIHI_LOG_INFO(g_logger) << addr->port();
}

int main(int argc, char** argv) {
    // test();
    test_ipv4();
    return 0;
}