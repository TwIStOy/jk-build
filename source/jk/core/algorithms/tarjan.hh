// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <algorithm>
#include <stack>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_join.h"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/build_rule_base.hh"
#include "jk/core/models/session.hh"
#include "jk/utils/logging.hh"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/transform.hpp"
#include "range/v3/view/unique.hpp"

namespace jk::core::algorithms {

struct StronglyConnectedComponent {
  std::vector<models::BuildRule *> Rules;
  std::vector<uint32_t> Deps;
};

inline auto Tarjan(models::Session *session, auto rg) {
  static auto logger = utils::Logger("tarjan");

  uint32_t dfncnt = 0;
  std::stack<models::BuildRule *> stack;
  std::vector<StronglyConnectedComponent> sccs;

  std::vector<uint32_t> low(models::__CurrentObjectId(), 0),
      dfn(models::__CurrentObjectId(), 0);
  std::vector<bool> in_stack(models::__CurrentObjectId(), false);

  auto dfs = [&](models::BuildRule *rule, auto &&dfs) -> void {
    ++dfncnt;

    auto u = rule->Base->ObjectId;

    low[u] = dfn[u] = dfncnt;
    stack.push(rule);
    in_stack[u] = true;

    for (auto n : rule->Dependencies) {
      auto v = n->Base->ObjectId;
      if (dfn[v] == 0) {
        dfs(n, dfs);
        low[u] = std::min<uint32_t>(low[u], low[v]);
      } else if (in_stack[n->Base->ObjectId]) {
        low[u] = std::min<uint32_t>(low[u], dfn[v]);
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

      absl::flat_hash_set<uint32_t> Deps;
      for (auto r : current_scc) {
        for (auto d : r->Dependencies) {
          assert(d->_scc_id >= 0);
          if (d->_scc_id != scc_id) {
            Deps.insert(d->_scc_id);
          }
        }
      }

      sccs.push_back(StronglyConnectedComponent{
          .Rules = std::move(current_scc), .Deps = Deps | ranges::to_vector});
    }
  };

  for (auto x : rg) {
    dfs(x, dfs);
  }

  for (auto i = 0; i < sccs.size(); i++) {
    logger->debug(
        "scc[{}], rules: [{}], deps: [{}]", i,
        absl::StrJoin(sccs[i].Rules, ",",
                      [](std::string *output, auto rule) {
                        output->append(std::to_string(rule->Base->ObjectId));
                        output->append(": ");
                        output->append(rule->Base->FullQualifiedName);
                      }),
        absl::StrJoin(sccs[i].Deps, ","));
  }
  logger->flush();

  return sccs;
}

}  // namespace jk::core::algorithms
