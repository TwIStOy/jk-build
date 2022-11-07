// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <vector>

namespace jk::core::executor {

class WorkerPool {
 public:
  explicit WorkerPool(uint32_t number = std::thread::hardware_concurrency());

  void Stop() {
    stop_.request_stop();
  }

  void Start() {
    for (auto i = 0u; i < number_; i++) {
      auto stoken = stop_.get_token();
      auto worker = [this, stoken = std::move(stoken)] {
        while (true) {
          if (stoken.stop_requested()) {
            break;
          }

          std::function<void()> func;
          {
            std::unique_lock lk(mutex_);
            if (queue_.empty()) {
              cond_.wait(lk);
            }

            if (stoken.stop_requested()) {
              break;
            }

            func = std::move(queue_.front());
            queue_.pop();
          }

          func();
        }
      };
      thrs_.emplace_back(worker);
    }
  }

  template<typename F>
  std::future<void> Push(F &&f) {
    std::unique_lock lk(mutex_);

    auto p = std::make_shared<std::promise<void>>();

    std::future<void> fur = p->get_future();
    queue_.emplace([f = std::forward<F>(f), p]() mutable {
      f();
      p->set_value();
    });
    cond_.notify_one();

    return fur;
  }

 private:
  uint32_t number_;
  std::vector<std::jthread> thrs_;

  std::mutex mutex_;
  std::condition_variable cond_;
  std::stop_source stop_;
  std::queue<std::function<void()>> queue_;
};

}  // namespace jk::core::executor
