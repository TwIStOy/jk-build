// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/common/path.hh"
#include "nlohmann/json.hpp"

namespace jk::core::cache {

class PersistentCache {
 public:
  enum class StorageType {
    Json,
    Binary,
  };

  struct CachingSlot {
    virtual ~CachingSlot() = default;

    virtual std::string TypeName() const = 0;

    /**
     * Check the persistent cache is expired or not.
     */
    virtual bool Expired(
        std::chrono::time_point<std::chrono::system_clock> ts) const = 0;

    /**
     * Key
     */
    virtual const std::string &Key() const = 0;

    virtual nlohmann::json ToJson() const = 0;

    virtual std::vector<uint8_t> ToBinary() const = 0;
  };

  using CachingSlotPtr = std::unique_ptr<CachingSlot>;

  PersistentCache(common::AbsolutePath root_path,
                  StorageType storage_type = StorageType::Binary);

  CachingSlot *Slot(const std::string &name) const;

  void Slot(const std::string &name, CachingSlotPtr slot);

  void Flush();

 private:
  void LoadJsonCache();
  void LoadBinaryCache();

  void FlushJsonCache() const;
  void FlushBinaryCache() const;

 private:
  common::AbsolutePath cache_path_;
  mutable std::unordered_map<std::string, CachingSlotPtr> caching_slots_;

 public:
  static std::unordered_map<std::string,
                            CachingSlotPtr (*)(nlohmann::json &doc)>
      JSONCreators;
  static std::unordered_map<std::string,
                            CachingSlotPtr (*)(uint8_t *data, uint32_t length)>
      BinaryCreators;
};

}  // namespace jk::core::cache

// vim: fdm=marker
