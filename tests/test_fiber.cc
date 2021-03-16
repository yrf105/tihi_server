#include "fiber/fiber.h"
#include "log/log.h"

tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void func1() {
    TIHI_LOG_DEBUG(g_logger) << "sub fiber start";
    tihi::Fiber::This()->YieldToHold();
    TIHI_LOG_DEBUG(g_logger) << "sub fiber end";
}

int main(int argc, char** argv) {
    tihi::Thread::SetName("main");
    tihi::Fiber::This(); // 初始化主协程
    TIHI_LOG_DEBUG(g_logger) << "main fiber start";
    tihi::Fiber::ptr fiber(new tihi::Fiber(func1)); // 创建子协程
    fiber->swapIn();
    TIHI_LOG_DEBUG(g_logger) << "main fiber mid";
    fiber->swapIn();
    TIHI_LOG_DEBUG(g_logger) << "main fiber end";
    return 0;
}