#ifndef TIHI_FIBER_FIBER_H_
#define TIHI_FIBER_FIBER_H_

#include <ucontext.h>

#include <functional>
#include <memory>

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    using ptr = std::shared_ptr<Fiber>;

    enum State {
        INIT = 0,
        READY = 1,
        EXEC = 2,
        HOLD = 3,
        TERM = 4,
    };

private:
    /*
    默认构造函数用来构造线程的主协程，主协程直接使用当前线程的上下文
    */
    Fiber();

public:
    /*
    制作一个新的协程，需要分配新的栈空间和指定执行函数，
    若成功，将在新的栈上执行指定函数，不会返回
    */
    Fiber(std::function<void()> cb, size_t stack_size = 0);
    ~Fiber();

    /*
    改变当前协程的执行函数，必须处于 INIT（未开始执行） 或者 TERM（执行完毕）
    状态
    */
    void reset(std::function<void()>);
    /*
    从其他协程（主协程）切换到当前协程（子协程）
    */
    void swapIn();
    /*
    当前协程（子协程）让出线程资源，切换到其他协程（主协程）
    */
    void swapOut();

    static ptr SetThis(Fiber* fiber);
    static ptr This();
    static void YieldToReady();
    static void YieldToHold();
    // 总的协程数
    static uint64_t TotalFiberCounts();

    static void MainFunc();

private:
    uint64_t id_;
    uint32_t stack_size_;
    State state_ = INIT;

    ucontext_t context_;
    void* stack_;

    std::function<void()> cb_;
};

#endif  // TIHI_FIBER_FIBER_H_