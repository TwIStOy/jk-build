// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <vector>

#include "jk/utils/str.hh"

namespace jk::core::models {

enum class RuleTypeEnum : uint8_t {
  kLibrary  = 1 << 0,
  kBinary   = 1 << 1,
  kTest     = 1 << 2,
  kExternal = 1 << 3,
  kCC       = 1 << 4,
};

#define TYPE_SET_GETTER(type)                                             \
  inline bool Is##type() const { return HasType(RuleTypeEnum::k##type); } \
  inline void Set##type() {                                               \
    value_ |= static_cast<uint8_t>(RuleTypeEnum::k##type);                \
  }

struct RuleType final : public utils::Stringifiable {
 public:
  RuleType() = default;
  inline RuleType(std::initializer_list<RuleTypeEnum> types) {
    for (auto tp : types) {
      SetType(tp);
    }
  }

  inline bool HasType(RuleTypeEnum tp) const {
    return value_ & static_cast<uint8_t>(tp);
  }

  inline void SetType(RuleTypeEnum tp) {
    value_ |= static_cast<uint8_t>(tp);
  }

  TYPE_SET_GETTER(Library);
  TYPE_SET_GETTER(Binary);
  TYPE_SET_GETTER(Test);
  TYPE_SET_GETTER(External);
  TYPE_SET_GETTER(CC);

  // inherited from |utils::Stringifiable|
  std::string gen_stringify_cache() const final;

 private:
  uint8_t value_{0};
};

inline std::string RuleType::gen_stringify_cache() const {
  std::vector<std::string> flags;
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kLibrary)) {
    flags.push_back("library");
  }
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kBinary)) {
    flags.push_back("binary");
  }
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kTest)) {
    flags.push_back("test");
  }
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kExternal)) {
    flags.push_back("external");
  }
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kCC)) {
    flags.push_back("cc");
  }
  return fmt::format("RuleType [{}]",
                     utils::JoinString(" | ", flags.begin(), flags.end()));
}

#undef TYPE_SET_GETTER

}  // namespace jk::core::models
