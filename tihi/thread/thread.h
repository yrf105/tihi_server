#ifndef TIHI_THREAD_THREAD_H_
#define TIHI_THREAD_THREAD_H_

#include <pthread.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>

#include "utils/noncopyable.h"
#include "utils/mutex.h"

namespace tihi {

class Thread : public Noncopyable {
public:
    using ptr = std::shared_ptr<Thread>;

    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t id() const { return id_; }
    const std::string& name() const { return name_; }


    static Thread* This();
    static const std::string& Name();
    static void SetName(const std::string& name);

    void join();

private:
    static void* run(void*);

private:
    std::function<void()> cb_;
    std::string name_;
    pid_t id_ = -1;
    pthread_t thread_ = 0;

    Semaphore semaphore_;
};

}  // namespace tihi

#endif  // THREAD_THREAD_H_