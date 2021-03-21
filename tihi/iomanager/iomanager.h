#ifndef TIHI_IOMANAGER_IOMANAGER_H_
#define TIHI_IOMANAGER_IOMANAGER_H_

#include <sys/epoll.h>

#include "scheduler/scheduler.h"
#include "timer/timer.h"

namespace tihi {

class IOManager : public Scheduler, public TimerManager {
public:
    using ptr = std::shared_ptr<IOManager>;
    using mutex_type = RWMutex;

    enum EventType {
        NONE = 0x0,
        READ = EPOLLIN,
        WRITE = EPOLLOUT,
    };

    IOManager(size_t threads = 1, bool use_caller = true,
              const std::string& name = "");
    ~IOManager();

    int addEvent(int fd, EventType type, std::function<void()> cb = nullptr);
    bool delEvent(int fd, EventType type);
    bool cancelEvent(int fd, EventType type);

    bool cancelAll(int fd);

    static IOManager* This();

protected:
    void tickle() override;
    bool stopping() override;
    bool stopping(uint64_t& timeout);
    void idle() override;
    void resize(size_t size);
    void onTimerInsertedAtFront() override;

private:
    struct Event {
        using mutex_type = Mutex;
        struct EventContext {
            Scheduler* scheduler_ = nullptr;
            Fiber::ptr fiber_;
            std::function<void()> cb_;
        };

        EventContext& event_context(EventType type);
        void resetEventContext(EventContext& ectx);
        void triggerEvent(EventType type);


        int fd_ = -1;
        EventType types_ = NONE;
        EventContext read_;
        EventContext write_;
        mutex_type mutex_;
    };

    int epfd_;
    int pipefd_[2];
    std::atomic<size_t> pending_event_counts_{0};
    std::vector<Event*> events_;
    mutex_type mutex_;    
};

} // namespace tihi

#endif // TIHI_IOMANAGER_IOMANAGER_H_