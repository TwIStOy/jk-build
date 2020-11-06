// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "jk/core/parser/parser_context.hh"
#include "jk/core/parser/type_system.hh"
#include "jk/utils/type_traits.hh"

namespace jk::core::parser {

template<typename ResultType>
struct ParserFragment {
  using result_t = std::optional<ResultType>;
  using context_t = std::pair<ParserContext, result_t>;

  using FragmentFunction = std::function<context_t(const ParserContext &)>;

  FragmentFunction Executor;

  inline auto operator()(const ParserContext &ctx) const {
    return Executor(ctx);
  }
};

/**
 * Combine two fragments
 */
template<typename T, typename U>
ParserFragment<typename TypePlus<T, U>::type> operator+(
    ParserFragment<T> lparser, ParserFragment<U> rparser) {
  using frag_type = ParserFragment<typename TypePlus<T, U>::type>;
  ParserFragment<typename TypePlus<T, U>::type> frag;

  frag.Executor = [lhs = std::move(lparser),
                   rhs = std::move(rparser)](const ParserContext &ctx) ->
      typename frag_type::context_t {
        auto [lresult_ctx, lresult_result] = lhs(ctx);
        if (lresult_result) {
          auto [rresult_ctx, rresult_result] = rhs(lresult_ctx);
          return {rresult_ctx,
                  {TypePlus<T, U>::Combine(lresult_result.value(),
                                           rresult_result.value())}};
        }
        return {lresult_ctx, {}};
      };

  return frag;
}

/**
 * Or two fragments.
 */
template<typename T, typename U>
ParserFragment<typename TypeOr<T, U>::type> operator|(
    ParserFragment<T> lparser, ParserFragment<U> rparser) {
  using frag_type = ParserFragment<typename TypeOr<T, U>::type>;
  ParserFragment<typename TypeOr<T, U>::type> frag;

  frag.Executor = [lhs = std::move(lparser),
                   rhs = std::move(rparser)](const ParserContext &ctx) ->
      typename frag_type::context_t {
        auto [lresult_ctx, lresult_result] = lhs(ctx);
        if (lresult_result) {
          return {lresult_ctx, {lresult_result.value()}};
        }

        auto [rresult_ctx, rresult_result] = rhs(lresult_ctx);
        if (rresult_result) {
          return {rresult_ctx, {rresult_result.value()}};
        } else {
          return {rresult_ctx, {}};
        }
      };

  return frag;
}

}  // namespace jk::core::parser

// vim: fdm=marker

