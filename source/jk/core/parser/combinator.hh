// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "jk/core/parser/parser.hh"

namespace jk::core::parser {

// // combine :: Parser a -> Parser b -> (a -> b -> c) -> Parser c
// template<typename P1, typename P2, typename F,
//          typename R = std::invoke_result_t<F, typename P1::result_t,
//                                            typename P2::result_t>>
// constexpr auto combine(P1 &&p1, P2 &&p2, F &&f) {
//   return [=](ParserInput s) -> ParserResult<R> {
//     auto r1 = p1(s);
//     if (!r1) {
//       return std::nullopt;
//     }
//     auto r2 = p2(r1->second);
//     if (!r2) {
//       return std::nullopt;
//     }
//     return std::make_pair(std::make_pair(r1->first, r2->first), r2->second);
//   };
// }
//
// // operator> :: Parser a -> Parser b -> Parser a
// template<typename P1, typename P2>
// constexpr auto operator>(P1 &&p1, P2 &&p2) {
//   return combine(std::forward<P1>(p1), std::forward<P2>(p2),
//                  [](auto &&l, auto) {
//                    return l;
//                  });
// }
//
// // operator< :: Parser a -> Parser b -> Parser b
// template<typename P1, typename P2>
// constexpr auto operator<(P1 &&p1, P2 &&p2) {
//   return combine(std::forward<P1>(p1), std::forward<P2>(p2),
//                  [](auto, auto &&r) {
//                    return r;
//                  });
// }
//
// // foldL :: Parser a -> b -> (b -> a -> b) -> ParserInput -> ParserResult b
// template<typename P, typename R, typename F>
// constexpr auto foldL(P &&p, R acc, F &&f, ParserInput in) -> ParserResult<R> {
//   while (true) {
//     auto r = p(in);
//     if (!r)
//       return std::make_pair(acc, in);
//     acc = f(acc, r->first);
//     in = r->second;
//   }
// };
//
// // many :: Parser a -> Parser monostate
// template<typename P>
// constexpr auto many(P &&p) {
//   return
//       [p = std::forward<P>(p)](ParserInput s) -> ParserResult<std::monostate> {
//         return foldL(
//             p, std::monostate{},
//             [](auto acc, auto) {
//               return acc;
//             },
//             s);
//       };
// };
//
// // atLeast :: Parser a -> b -> (b -> a -> b) -> Parser b
// template<typename P, typename R, typename F>
// constexpr auto atLeast(P &&p, R &&init, F &&f) {
//   static_assert(
//       std::is_same_v<std::invoke_result_t<F, R, typename P::result_t>, R>,
//       "type mismatch!");
//   return [p = std::forward<P>(p), f = std::forward<F>(f),
//           init = std::forward<R>(init)](ParserInput s) -> ParserResult<R> {
//     auto r = p(s);
//     if (!r)
//       return std::nullopt;
//     return foldL(p, f(init, r->first), f, r->second);
//   };
// };
//
// // option :: Parser a -> a -> Parser a
// template<typename P, typename R = typename P::result_t>
// constexpr auto option(P &&p, R &&defaultV) {
//   return [=](ParserInput s) -> ParserResult<R> {
//     auto r = p(s);
//     if (!r)
//       return make_pair(defaultV, s);
//     return r;
//   };
// };
//
}  // namespace jk::core::parser

// vim: fdm=marker

