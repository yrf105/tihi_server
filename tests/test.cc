#include <iostream>

#include "log/log.h"
#include "utils/utils.h"

int main(int argc, char** argv) {
    // std::cout << "Hello World!" << std::endl;

    tihi::Logger::ptr logger = tihi::Logger::ptr(new tihi::Logger("root"));
    logger->addAppender(tihi::LogAppender::ptr(new tihi::StdOutLogAppender()));

    tihi::LogEvent::ptr event = tihi::LogEvent::ptr(new tihi::LogEvent(
        time(0), 0, __FILE__, __LINE__, tihi::ThreadId(), tihi::FiberId(),
        logger, tihi::LogLevel::DEBUG, tihi::Thread::Name()));
    event->content() << "hello tihi server";

    logger->log(tihi::LogLevel::DEBUG, event);

    TIHI_LOG_INFO(logger) << "info";
    TIHI_LOG_FATAL(logger) << "fatal";

    TIHI_LOG_FMT_INFO(logger, "format %s", "info");

    auto l = tihi::LoggerMgr::GetInstance()->logger("xx");
    TIHI_LOG_FATAL(l) << "fatal";

    return 0;
}