#ifndef TIHI_UTILS_NONCOPYABLE_H_
#define TIHI_UTILS_NONCOPYABLE_H_

namespace tihi {
class Noncopyable {
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;

private:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable(const Noncopyable&&) = delete;
    const Noncopyable& operator=(const Noncopyable&) = delete; 

};

}  // namespace tihi

#endif  // TIHI_UTILS_NONCOPYABLE_H_