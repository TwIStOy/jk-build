// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <variant>

#include "jk/utils/cpp_features.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

class IncludeArgument : public utils::Stringifiable {
 public:
  enum class Placehoder {
    WorkingFolder,
  };

  // NOLINTNEXTLINE
  IncludeArgument(std::string trivial);

  // NOLINTNEXTLINE
  IncludeArgument(Placehoder p);

  bool IsTrivial() const;

  bool IsPlacehoder() const;

  const std::string &StrValue() const;

  Placehoder PlacehoderValue() const;

  // inherited from |utils::Stringifiable|
  std::string Stringify() const final;

  ALWAYS_INLINE bool operator==(const IncludeArgument &rhs) {
    return data_ == rhs.data_;
  }

  ALWAYS_INLINE bool operator<(const IncludeArgument &rhs) {
    if (data_.index() != rhs.data_.index()) {
      return data_.index() < rhs.data_.index();
    }
    if (data_.index() == 0) {
      return std::get<0>(data_) < std::get<0>(rhs.data_);
    }
    return static_cast<uint32_t>(std::get<1>(data_)) <
           static_cast<uint32_t>(std::get<1>(rhs.data_));
  }

 private:
  std::variant<std::string, Placehoder> data_;
};

ALWAYS_INLINE bool IncludeArgument::IsTrivial() const {
  return data_.index() == 0;
}

ALWAYS_INLINE bool IncludeArgument::IsPlacehoder() const {
  return data_.index() == 1;
}

ALWAYS_INLINE const std::string &IncludeArgument::StrValue() const {
  return std::get<0>(data_);
}

ALWAYS_INLINE IncludeArgument::Placehoder IncludeArgument::PlacehoderValue()
    const {
  return std::get<1>(data_);
}

}  // namespace jk::rules::cc

// vim: fdm=marker

