#include "mutex.h"
#include <stdexcept>

namespace tihi {

Semaphore::Semaphore(const uint32_t count) {
    if (sem_init(&semaphore_, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() {
    sem_destroy(&semaphore_);
}

void Semaphore::wait() {
    if (sem_wait(&semaphore_)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if (sem_post(&semaphore_)) {
        throw std::logic_error("sem_post error");
    }
}

} // namespace tihi