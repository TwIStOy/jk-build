// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace jk::utils {

template<typename T, typename U, typename = typename T::const_iterator,
         typename = typename U::const_iterator,
         typename value_type = typename T::value_type,
         typename = std::enable_if_t<
             std::is_same_v<typename T::value_type, typename U::value_type>>>
bool SameArrayInOrder(const T &lhs, const U &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  auto lit = lhs.begin();
  auto rit = rhs.begin();
  for (; lit != lhs.end(); ++lit, ++rit) {
    if (*lit != *rit) {
      return false;
    }
  }
  return true;
}

template<typename T, typename U, typename = typename T::const_iterator,
         typename = typename U::const_iterator,
         typename value_type = typename T::value_type,
         typename = std::enable_if_t<
             std::is_same_v<typename T::value_type, typename U::value_type>>>
bool SameArray(const T &lhs, const U &rhs) {
  std::set<value_type> lparts{lhs.begin(), lhs.end()};
  std::set<value_type> rparts{rhs.begin(), rhs.end()};

  if (lparts.size() != rparts.size()) {
    return false;
  }

  auto it1 = lparts.begin();
  auto it2 = rparts.begin();
  for (; it1 != lparts.end(); ++it1, ++it2) {
    if (*it1 != *it2) {
      return false;
    }
  }

  return true;
}

template<template<typename...> class T>
bool SameArray(const std::string &str, const T<std::string> &vec) {
  std::set<std::string> parts;
  std::set<std::string> parts2{vec.begin(), vec.end()};

  std::stringstream iss(str);
  std::string line;
  while (std::getline(iss, line, ' ')) {
    parts.insert(line);
  }

  if (vec.size() != parts.size()) {
    return false;
  }

  auto it1 = parts.begin();
  auto it2 = parts2.begin();
  for (; it1 != parts.end(); ++it1, ++it2) {
    if (*it1 != *it2) {
      return false;
    }
  }

  return true;
}

namespace __detail {

template<typename T>
void ConcatArraysImpl(std::vector<T> *res) {
}

template<typename T, typename First, typename... Args,
         typename = std::enable_if_t<
             std::is_same_v<typename First::value_type, T> &&
             (std::is_same_v<typename Args::value_type, T> && ...)>>
void ConcatArraysImpl(std::vector<T> *res, const First &first,
                      const Args &...rest) {
  res->insert(res->end(), std::begin(first), std::end(first));
  ConcatArraysImpl(res, rest...);
}

}  // namespace __detail

template<
    typename First, typename... Args,
    typename = std::enable_if_t<(
        std::is_same_v<typename First::value_type, typename Args::value_type> &&
        ...)>,
    typename ValueType = typename First::value_type>
std::vector<ValueType> ConcatArrays(const First &first, const Args &...args) {
  std::vector<ValueType> res;
  __detail::ConcatArraysImpl(&res, first, args...);
  return res;
}

}  // namespace jk::utils

// vim: fdm=marker
