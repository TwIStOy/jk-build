// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <algorithm>
#include <stack>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/build_rule_base.hh"
#include "jk/core/models/session.hh"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/unique.hpp"

namespace jk::core::algorithms {

struct StronglyConnectedComponent {
  std::vector<models::BuildRule *> Rules;
  std::vector<uint32_t> Deps;
};

inline auto Tarjan(models::Session *session, auto rg) {
  uint32_t dfncnt = 0;
  std::stack<models::BuildRule *> stack;
  std::vector<StronglyConnectedComponent> sccs;

  std::vector<uint32_t> low(models::__CurrentObjectId(), 0),
      dfn(models::__CurrentObjectId(), 0);
  std::vector<bool> in_stack(models::__CurrentObjectId(), false);

  auto dfs = [&](models::BuildRule *rule, auto &&dfs) {
    low[rule->Base->ObjectId] = dfn[rule->Base->ObjectId] = ++dfncnt;
    stack.push(rule);
    in_stack[rule->Base->ObjectId] = true;

    for (auto n : rule->Dependencies) {
      if (dfn[n->Base->ObjectId] > 0) {
        dfs(n, dfs);
        low[n->Base->ObjectId] =
            std::min(low[rule->Base->ObjectId], low[n->Base->ObjectId]);
      } else if (in_stack[n->Base->ObjectId]) {
        low[n->Base->ObjectId] =
            std::min(low[rule->Base->ObjectId], dfn[n->Base->ObjectId]);
      }
    }

    if (low[rule->Base->ObjectId] == dfn[rule->Base->ObjectId]) {
      auto scc_id = sccs.size();
      std::vector<models::BuildRule *> current_scc;
      while (stack.top() != rule) {
        stack.top()->_scc_id = scc_id;
        current_scc.push_back(stack.top());
        in_stack[stack.top()->Base->ObjectId] = false;
        stack.pop();
      }
      assert(stack.top() == rule);
      stack.top()->_scc_id = scc_id;
      current_scc.push_back(stack.top());
      in_stack[stack.top()->Base->ObjectId] = false;
      stack.pop();

      sccs.push_back(
          StronglyConnectedComponent{.Rules = std::move(current_scc)});
    }
  };

  rg | ranges::views::for_each([&](auto x) {
    dfs(x, dfs);
  });

  rg | ranges::views::for_each([&](models::BuildRule *x) {
    for (auto n : x->Dependencies) {
      if (x->_scc_id != n->_scc_id) {
        sccs[x->_scc_id].Deps.push_back(n->_scc_id);
      }
    }
  });

  for (auto &scc : sccs) {
    scc.Deps = scc.Deps | ranges::views::unique | ranges::to_vector;
  }

  return sccs;
}

}  // namespace jk::core::algorithms
