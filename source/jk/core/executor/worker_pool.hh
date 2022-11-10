// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace jk::core::executor {

class WorkerPool {
 public:
  explicit WorkerPool(uint32_t number = std::thread::hardware_concurrency())
      : number_(number) {
  }

  ~WorkerPool();

  void Start();

  bool is_abort() const;

  void set_abort_flag();

  template<class F, class R = std::invoke_result_t<F>>
  std::future<R> Push(F f) {
    std::packaged_task<R()> task(std::move(f));
    auto ret = task.get_future();
    if (queue_.push(std::packaged_task<void()>(std::move(task)))) {
      return ret;
    } else {
      return {};
    }
  }

 private:
  void run();

  template<typename T>
  struct threadsafe_queue {
    [[nodiscard]] std::optional<T> pop() {
      std::unique_lock lk(mutex_);
      cond_.wait(lk, [this] {
        return is_abort() || !data_.empty();
      });

      if (is_abort()) {
        return {};
      }

      auto r = std::move(data_.front());
      data_.pop_front();
      cond_.notify_all();
      return r;
    }

    bool push(T t) {
      std::unique_lock lk(mutex_);
      if (is_abort())
        return false;
      data_.push_back(std::move(t));
      cond_.notify_one();
      return true;
    }

    void set_abort_flag() {
      std::unique_lock lk(mutex_);
      aborted_ = true;
      data_.clear();
      cond_.notify_all();
    }

    bool is_abort() const {
      return aborted_;
    }

    void wait_until_empty() {
      std::unique_lock lk(mutex_);
      cond_.wait(lk, [this] {
        return data_.empty();
      });
    }

    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> aborted_{false};
    std::deque<T> data_;
  };

 private:
  uint32_t number_;
  std::vector<std::jthread> thrs_;

  threadsafe_queue<std::packaged_task<void()>> queue_;
};

}  // namespace jk::core::executor
