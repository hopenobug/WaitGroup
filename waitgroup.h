#ifndef _CPP_WAIT_GROUP_H_
#define _CPP_WAIT_GROUP_H_

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>

class WaitGroup {
  public:
    // Constructs the WaitGroup with the specified initial count.
    inline WaitGroup(unsigned int initialCount = 0);

    inline void Add(unsigned int count = 1) const;

    // Returns true if the internal count has reached zero.
    inline bool Done() const;

    inline void Wait() const;

    inline bool WaitFor(unsigned int milliseconds) const;

    class DoneGuard {
      public:
        DoneGuard(const WaitGroup &_wg);
        ~DoneGuard();

        DoneGuard(const DoneGuard &) = delete;
        DoneGuard &operator=(const DoneGuard &) = delete;

      private:
        const WaitGroup &wg;
    };

  private:
    struct Data {
        std::atomic<unsigned int> count = {0};
        std::condition_variable cv;
        std::mutex mutex;
    };
    const std::shared_ptr<Data> data;
};

WaitGroup::WaitGroup(unsigned int initialCount /* = 0 */) : data(std::make_shared<Data>()) { data->count = initialCount; }

void WaitGroup::Add(unsigned int count /* = 1 */) const { data->count += count; }

bool WaitGroup::Done() const {
    assert(data->count > 0);

    if (--data->count == 0) {
        // this lock is necessary, accrodding to
        // https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables
        std::unique_lock<std::mutex> lock(data->mutex);
        data->cv.notify_all();
        return true;
    }
    return false;
}

void WaitGroup::Wait() const {
    if (data->count == 0) {
        return;
    }
    std::unique_lock<std::mutex> lock(data->mutex);
    data->cv.wait(lock, [this] { return data->count == 0; });
}

bool WaitGroup::WaitFor(unsigned int milliseconds) const {
    if (data->count == 0) {
        return true;
    }
    std::unique_lock<std::mutex> lock(data->mutex);
    return data->cv.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] { return data->count == 0; });
}

WaitGroup::DoneGuard::DoneGuard(const WaitGroup &_wg) : wg(_wg) {}

inline WaitGroup::DoneGuard::~DoneGuard() { wg.Done(); }

#endif