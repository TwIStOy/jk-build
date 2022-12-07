// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/executor/worker_pool.hh"

namespace jk::core::executor {

bool WorkerPool::is_abort() const {
  return queue_.is_abort();
}

auto WorkerPool::set_abort_flag() -> void {
  queue_.set_abort_flag();
}

void WorkerPool::run() {
  while (auto f = queue_.pop()) {
    (*f)();
  }
}

void WorkerPool::Start() {
  for (auto i = 0u; i < number_; i++) {
    thrs_.emplace_back(&WorkerPool::run, this);
  }
}

WorkerPool::~WorkerPool() {
  queue_.wait_until_empty();
  queue_.set_abort_flag();
  thrs_.clear();
}

}  // namespace jk::core::executor
