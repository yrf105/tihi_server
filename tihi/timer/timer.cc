#include "timer.h"
#include "log/log.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager* timer_manager)
    : ms_(ms), cb_(cb), recurring_(recurring), timer_manager_(timer_manager) {
    next_ = MS() + ms_;
}

Timer::Timer(uint64_t next) : next_(next) {
}

bool Timer::cancel() {
    TimerManager::rwmutex_type::write_lock lock(timer_manager_->mutex_);
    if (cb_) {
        cb_ = nullptr;
        auto it = timer_manager_->timers_.find(shared_from_this());
        if (it == timer_manager_->timers_.end()) {
            return false;
        }
        timer_manager_->timers_.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::rwmutex_type::write_lock lock(timer_manager_->mutex_);
    if (!cb_) {
        return false;
    }

    auto it = timer_manager_->timers_.find(shared_from_this());
    if (it == timer_manager_->timers_.end()) {
        return false;
    }
    timer_manager_->timers_.erase(it);
    next_ = MS() + ms_;
    timer_manager_->timers_.insert(shared_from_this());

    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == ms_ && !from_now) {
        return true;
    }

    TimerManager::rwmutex_type::write_lock lock(timer_manager_->mutex_);
    if (!cb_) {
        return false;
    }

    auto it = timer_manager_->timers_.find(shared_from_this());
    if (it == timer_manager_->timers_.end()) {
        return false;
    }

    timer_manager_->timers_.erase(it);

    uint64_t start = 0;
    if (from_now) {
        start = MS();
    } else {
        start = next_ - ms_;
    }
    ms_ = ms;
    next_ = start + ms_;
    
    timer_manager_->addTimer(shared_from_this(), lock);

    return true;
}


bool Timer::Cmp::operator()(const Timer::ptr& lhs,
                            const Timer::ptr& rhs) const {
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    if (lhs->next_ < rhs->next_) {
        return true;
    }
    if (lhs->next_ > rhs->next_) {
        return false;
    }

    return lhs.get() < rhs.get();
}

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::addTimer(Timer::ptr timer,
                            TimerManager::rwmutex_type::write_lock& lock) {
    auto it = timers_.insert(timer).first;
    /**
     * 设置 tickled 避免发生频繁修改时，每次都唤醒
    */
    bool at_front = (it == timers_.begin()) && !tickled_;
    if (at_front) {
        tickled_ = true;
    }
    lock.unlock();

    if (at_front) {
        onTimerInsertedAtFront();
    }
}


Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    rwmutex_type::write_lock lock(mutex_);
    
    addTimer(timer, lock);
    /**
     * 将加入的定时器返回给客户端，让客户端有机会将其删除
     */
    return timer;
}

static void onTimer(std::weak_ptr<void> cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = cond.lock();
    if (tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> cond,
                                           bool recurring) {
    return addTimer(ms, std::bind(&onTimer, cond, cb), recurring);
}

uint64_t TimerManager::nextTimerTime() {
    rwmutex_type::read_lock lock(mutex_);
    tickled_ = false;
    if (timers_.empty()) {
        return ~0ul;
    }

    const Timer::ptr& next = *(timers_.begin());
    uint64_t now = MS();
    if (now >= next->next_) {
        return 0;
    } else {
        return next->next_ - now;
    }
}

void TimerManager::expiredTimerCb(std::vector<std::function<void()>>& cbs) {
    uint64_t now_time = MS();
    {
        rwmutex_type::read_lock lock(mutex_);
        if (timers_.empty()) {
            return;
        }
    }

    std::vector<Timer::ptr> expired;

    rwmutex_type::write_lock lock(mutex_);
    Timer::ptr now(new Timer(now_time));

    bool rollover = detectRollOver(now_time);
    if (!rollover && (*timers_.begin())->next_ > now_time) {
        return ;
    }

    auto it = rollover ? timers_.end() : timers_.upper_bound(now);
    // auto it = timers_.end();

    expired.insert(expired.begin(), timers_.begin(), it);    
    timers_.erase(timers_.begin(), it);
    cbs.reserve(expired.size());

    for (auto& pt : expired) {
        cbs.push_back(pt->cb_);
        if (pt->recurring_) {
            pt->next_ = now_time + pt->ms_;
            timers_.insert(pt);
        } else {
            pt->cb_ = nullptr;
        }
    }

    // TIHI_LOG_DEBUG(g_sys_logger) << "timers_: " << timers_.size() << " expired: " << expired.size();
}

bool TimerManager::detectRollOver(uint64_t now_time) {
    bool rollover = false;
    if (previouseTime < now_time && previouseTime < now_time - (60 * 60 * 1000)) {
        rollover = true;
    }

    previouseTime = now_time;
    return rollover;
}

bool TimerManager::hasTimer() {
    rwmutex_type::read_lock lock(mutex_);
    return !timers_.empty();
}


}  // namespace tihi