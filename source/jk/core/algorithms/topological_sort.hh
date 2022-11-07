// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <vector>

#include "jk/core/algorithms/tarjan.hh"

namespace jk::core::algorithms {

inline auto topological_sort(
    const std::vector<StronglyConnectedComponent> &sccs) {
  std::vector<uint32_t> ingress_cnt(sccs.size(), 0);
  for (const auto &scc : sccs) {
    for (auto id : scc.Deps) {
      ingress_cnt[id]++;
    }
  }

  std::queue<uint32_t> Q;
  for (auto i = 0u; i < sccs.size(); i++) {
    if (ingress_cnt[i] == 0) {
      Q.push(i);
    }
  }

  std::vector<uint32_t> sorted;

  while (!Q.empty()) {
    auto id = Q.front();
    Q.pop();

    sorted.push_back(id);
    for (auto n : sccs[id].Deps) {
      ingress_cnt[n]--;
      if (ingress_cnt[n] == 0) {
        Q.push(n);
      }
    }
  }

  return sorted;
}

}  // namespace jk::core::algorithms
