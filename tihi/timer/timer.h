#ifndef TIHI_TIMER_TIMER_
#define TIHI_TIMER_TIMER_

#include <functional>
#include <memory>
#include <set>

#include "utils/mutex.h"
#include "utils/utils.h"

namespace tihi {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
    friend TimerManager;

public:
    using ptr = std::shared_ptr<Timer>;

    bool cancel();
    bool refresh();
    bool reset(uint64_t ms, bool from_now = true);

private:
    Timer(uint64_t ms, std::function<void()> cb, bool recurring,
          TimerManager* timer_manager);
    Timer(uint64_t next);

    struct Cmp {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };

private:
    uint64_t ms_ = 0;
    std::function<void()> cb_;
    bool recurring_ = false;
    TimerManager* timer_manager_ = nullptr;

    uint64_t next_ = 0;
};

class TimerManager {
    friend Timer;

public:
    using rwmutex_type = RWMutex;

    TimerManager();
    virtual ~TimerManager();

    void addTimer(Timer::ptr timer, rwmutex_type::write_lock& lock);
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring = false);
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> cond,
                                 bool recurring = false);

    uint64_t nextTimerTime();
    void expiredTimerCb(std::vector<std::function<void()>>& cbs);

    bool detectRollOver(uint64_t now_time);
    bool hasTimer();
protected:
    virtual void onTimerInsertedAtFront() = 0;

private:
    std::set<Timer::ptr, Timer::Cmp> timers_;
    rwmutex_type mutex_;
    bool tickled_ = false;
    uint64_t previouseTime = MS();
};

}  // namespace tihi

#endif  // TIHI_TIMER_TIMER_