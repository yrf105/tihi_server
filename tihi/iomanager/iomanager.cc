#include "iomanager.h"

#include <fcntl.h>

#include <cstring>

#include "log/log.h"
#include "utils/macro.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

void IOManager::Event::triggerEvent(IOManager::EventType type) {
    TIHI_ASSERT((types_ & type));
    EventContext& ectx = event_context(type);
    if (ectx.cb_) {
        ectx.scheduler_->schedule(&(ectx.cb_));
    } else if (ectx.fiber_) {
        ectx.scheduler_->schedule((&ectx.fiber_));
    }

    ectx.scheduler_ = nullptr;
}

IOManager::Event::EventContext& IOManager::Event::event_context(
    IOManager::EventType type) {
    switch (type) {
        case READ: {
            return read_;
        }
        case WRITE: {
            return write_;
        }
        default:
            TIHI_ASSERT2((false), "event type error");
    }

    // return NONE;
}

void IOManager::Event::resetEventContext(IOManager::Event::EventContext& ectx) {
    ectx.fiber_.reset();
    ectx.cb_ = nullptr;
    ectx.scheduler_ = nullptr;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name) {
    epfd_ = epoll_create(5000);
    TIHI_ASSERT((epfd_ >= 0));

    int ret = pipe(pipefd_);
    TIHI_ASSERT((ret == 0));

    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    ret = fcntl(pipefd_[0], F_SETFL, O_NONBLOCK);
    TIHI_ASSERT(ret == 0);
    event.data.fd = pipefd_[0];
    event.events = EPOLLIN | EPOLLET;

    ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, pipefd_[0], &event);
    TIHI_ASSERT((ret == 0));

    resize(32);

    start();
}

IOManager::~IOManager() {
    stop();
    close(epfd_);
    close(pipefd_[0]);
    close(pipefd_[1]);

    for (auto e : events_) {
        if (e) {
            delete (e);
        }
    }
}

int IOManager::addEvent(int fd, EventType type, std::function<void()> cb) {
    mutex_type::read_lock lock(mutex_);
    Event* event = nullptr;
    if (events_.size() > (size_t)fd) {
        event = events_[fd];
        lock.unlock();
    } else {
        lock.unlock();
        mutex_type::write_lock lock2(mutex_);
        resize(fd * 1.5);
        event = events_[fd];
    }

    Event::mutex_type::mutex lock2(event->mutex_);
    if (event->types_ & type) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "addEvent error: fd: " << fd << " event.type: " << event->types_
            << " type: " << type;
        TIHI_ASSERT(((event->types_ & type) == 0));
    }

    int op = event->types_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.data.ptr = event;
    epevent.events = event->types_ | EPOLLET | type;
    // int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    // TIHI_ASSERT(ret == 0);
    int rt = epoll_ctl(epfd_, op, fd, &epevent);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "rt: " << rt << " epoll_ctl(" << epfd_ << ", " << op << ", "
            << fd << ", " << epevent.events << ") errno: " << errno << " - "
            << strerror(errno);
        return -1;
    }

    ++pending_event_counts_;
    event->types_ = (EventType)(event->types_ | type);
    event->fd_ = fd;
    Event::EventContext& event_context = event->event_context(type);
    TIHI_ASSERT((event_context.cb_ == nullptr &&
                 event_context.fiber_ == nullptr &&
                 event_context.scheduler_ == nullptr));
    event_context.scheduler_ = Scheduler::This();
    if (cb) {
        event_context.cb_.swap(cb);
    } else {
        event_context.fiber_ = Fiber::This();
        TIHI_ASSERT((event_context.fiber_->state() == Fiber::EXEC));
    }

    return 0;
}

/**
 * type 中要删掉的类型，位为 1
 */
bool IOManager::delEvent(int fd, EventType type) {
    mutex_type::read_lock lock(mutex_);
    if (events_.size() <= (size_t)fd) {
        return false;
    }
    Event* event = events_[fd];
    lock.unlock();

    Event::mutex_type::mutex lock2(event->mutex_);
    /**
     * type 中为 1 的位，都是 event->types_ 中为 0 的位
     * 意味着要删除的类型本来就不存在
     */
    if (!(event->types_ & type)) {
        return false;
    }

    /**
     * 设定要删除的位在 new_type 中会为 0，其他位保持不变
     */
    EventType new_types = (EventType)(event->types_ & ~type);
    /**
     * EPOLL_CTL_DEL 会忽略 event（第四个参数）
     */
    int op = new_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = EPOLLET | new_types;
    epevent.data.ptr = event;
    int rt = epoll_ctl(epfd_, op, fd, &epevent);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "rt: " << rt << " epoll_ctl(" << epfd_ << ", " << op << ", "
            << fd << ", " << epevent.events << ") errno: " << errno << " - "
            << strerror(errno);
        return false;
    }

    --pending_event_counts_;
    event->types_ = new_types;
    Event::EventContext& event_context = event->event_context(type);
    event->resetEventContext(event_context);

    return true;
}

bool IOManager::cancelEvent(int fd, EventType type) {
    mutex_type::read_lock lock(mutex_);
    if (events_.size() <= (size_t)fd) {
        return false;
    }
    Event* event = events_[fd];
    lock.unlock();

    Event::mutex_type::mutex lock2(event->mutex_);
    /**
     * type 中为 1 的位，都是 event->types_ 中为 0 的位
     * 意味着要删除的类型本来就不存在
     */
    if (!(event->types_ & type)) {
        return false;
    }

    /**
     * 设定要删除的位在 new_type 中会为 0，其他位保持不变
     */
    EventType new_types = (EventType)(event->types_ & ~type);
    /**
     * EPOLL_CTL_DEL 会忽略 event（第四个参数）
     */
    int op = new_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = EPOLLET | new_types;
    epevent.data.ptr = event;
    int rt = epoll_ctl(epfd_, op, fd, &epevent);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "rt: " << rt << " epoll_ctl(" << epfd_ << ", " << op << ", "
            << fd << ", " << epevent.events << ") errno: " << errno << " - "
            << strerror(errno);
        return false;
    }

    event->triggerEvent(type);
    --pending_event_counts_;

    return true;
}

bool IOManager::cancelAll(int fd) {
    mutex_type::read_lock lock(mutex_);
    if (events_.size() <= static_cast<size_t>(fd)) {
        return false;
    }
    Event* event = events_[fd];
    lock.unlock();

    Event::mutex_type::mutex lock2(event->mutex_);
    /**
     * 该描述符本来就不存在
     */
    if (!(event->types_)) {
        return false;
    }

    int rt = epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "rt: " << rt << " epoll_ctl(" << epfd_ << ", " << EPOLL_CTL_DEL
            << ", " << fd << ", " << 0 << ") errno: " << errno << " - "
            << strerror(errno);
        return false;
    }

    if (event->types_ & READ) {
        event->triggerEvent(READ);
        --pending_event_counts_;
    }
    if (event->types_ & WRITE) {
        event->triggerEvent(WRITE);

        --pending_event_counts_;
    }

    event->fd_ = -1;

    TIHI_ASSERT((event->types_ == 0));

    return true;
}

IOManager* IOManager::This() {
    return dynamic_cast<IOManager*>(Scheduler::This());
}

void IOManager::tickle() {
    if (hasIdleThread()) {
        int ret = ::write(pipefd_[1], "T", 1);
        TIHI_ASSERT((ret == 1));
    }
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = nextTimerTime();
    return pending_event_counts_ == 0 && Scheduler::stopping() && !hasTimer();
}


bool IOManager::stopping() {
    uint64_t timeout;
    return stopping(timeout);
}

void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_event(
        events, [](epoll_event* ptr) { delete[] ptr; });

    while (true) {
        int ret = 0;
        uint64_t time_out = nextTimerTime();

        if (stopping(time_out) && time_out == ~0ul) {
            TIHI_LOG_INFO(g_sys_logger) << "idle exits";
            break;
        }

        do {
            static const int MAX_TIMEOUT = 3000;

            if (time_out != ~0ul) {
                time_out = std::min((int)time_out, MAX_TIMEOUT);
            } else {
                time_out = MAX_TIMEOUT;
            }

            ret = epoll_wait(epfd_, events, 64, (int)time_out);
            if (ret == -1 && errno == EINTR) {
            } else {
                break;
            }
        } while (true);

        /**
         * 将所有超时任务加入任务队列
        */
        std::vector<std::function<void()>> cbs;
        expiredTimerCb(cbs);
        if (!cbs.empty()) {
            // TIHI_LOG_DEBUG(g_sys_logger) << "size = " << cbs.size();
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (int i = 0; i < ret; ++i) {
            epoll_event& event = events[i];
            if (event.data.fd == pipefd_[0]) {
                char dummy;
                while ((read(pipefd_[0], &dummy, 1) == 1))
                    ;
                continue;
            }

            Event* e = (Event*)event.data.ptr;
            Event::mutex_type::mutex lock(e->mutex_);

            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }

            int real_types = NONE;
            if (event.events & EPOLLIN) {
                real_types |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_types |= WRITE;
            }

            if ((e->types_ & real_types) == NONE) {
                continue;
            }

            int left_type = e->types_ & ~real_types;
            int op = left_type ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_type;
            int ret2 = epoll_ctl(epfd_, op, e->fd_, &event);
            if (ret2) {
                TIHI_LOG_ERROR(g_sys_logger)
                    << "rt: " << ret2 << " epoll_ctl(" << epfd_ << ", " << op
                    << ", " << e->fd_ << ", " << event.events
                    << ") errno: " << errno << " - " << strerror(errno);
                continue;
            }

            if (real_types & READ) {
                e->triggerEvent(READ);
                --pending_event_counts_;
            }
            if (real_types & WRITE) {
                e->triggerEvent(WRITE);
                --pending_event_counts_;
            }
        }

        Fiber::ptr curr = Fiber::This();
        Fiber* raw_ptr = curr.get();
        curr.reset();

        raw_ptr->swapOut();
    }
}

void IOManager::resize(size_t size) {
    size_t old_size = events_.size();

    events_.resize(size);

    for (size_t i = old_size; i < events_.size(); ++i) {
        if (!events_[i]) {
            events_[i] = new Event;
            events_[i]->fd_ = i;
        }
    }
}

void IOManager::onTimerInsertedAtFront() { tickle(); }

}  // namespace tihi