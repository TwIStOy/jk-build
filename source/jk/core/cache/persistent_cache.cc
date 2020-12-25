// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/cache/persistent_cache.hh"

#include <boost/filesystem/operations.hpp>
#include <chrono>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::core::cache {

static auto logger = utils::Logger("cache");

PersistentCache::PersistentCache(common::AbsolutePath root_path,
                                 StorageType storage_type)
    : cache_path_(std::move(root_path)) {
  switch (storage_type) {
    case StorageType::Json: {
      LoadJsonCache();
      return;
    }
    case StorageType::Binary: {
      LoadBinaryCache();
      return;
    }
  }
}

PersistentCache::CachingSlot *PersistentCache::Slot(
    const std::string &name) const {
  auto now = std::chrono::system_clock::now();

  auto it = caching_slots_.find(name);
  if (it == caching_slots_.end()) {
    return nullptr;
  }
  if (it->second->Expired(now)) {
    caching_slots_.erase(it);
    return nullptr;
  }
  return it->second.get();
}

void PersistentCache::Slot(const std::string &name, CachingSlotPtr slot) {
  caching_slots_[name] = std::move(slot);
}

void PersistentCache::FlushJsonCache() const {
  // write to temporary file first
  auto temp_file =
      cache_path_.Sub("jk.cache.json." + utils::RandomAlphaNumString());

  nlohmann::json res;
  for (const auto &[key, slot] : caching_slots_) {
    (void)key;
    nlohmann::json doc;
    doc["type"] = slot->TypeName();
    doc["value"] = slot->ToJson();
    res.push_back(std::move(doc));
  }

  {
    std::ofstream ofs(temp_file.Stringify());
    ofs << res;
    ofs.flush();
  }

  auto fp = cache_path_.Sub("jk.cache.json");
  boost::filesystem::rename(temp_file.Path, fp.Path);
}

void PersistentCache::FlushBinaryCache() const {
  auto temp_file =
      cache_path_.Sub("jk.cache.binary." + utils::RandomAlphaNumString());

  {
    std::ofstream ofs(temp_file.Stringify());
    for (const auto &[key, slot] : caching_slots_) {
      (void)key;
      auto data = slot->ToBinary();
      ofs << slot->TypeName() << ","
          << utils::Base64Encode(data.data(), data.size()) << std::endl;
    }
    ofs.flush();
  }

  auto fp = cache_path_.Sub("jk.cache.binary");
  boost::filesystem::rename(temp_file.Path, fp.Path);
}

void PersistentCache::LoadJsonCache() {
  auto fp = cache_path_.Sub("jk.cache.json");
  std::ifstream ifs(fp.Stringify());

  try {
    nlohmann::json doc;
    ifs >> doc;

    auto name = doc["type"];
    auto it = JSONCreators.find(name);
    if (it == JSONCreators.end()) {
      throw JKBuildError("TypeName {} not registered", name);
    }
    auto slot = (it->second)(doc["value"]);
    caching_slots_[slot->Key()] = std::move(slot);
  } catch (std::exception &e) {
    logger->warn("Parse json cache failed. Ignore it! {}", e.what());
    caching_slots_.clear();
  }
}

void PersistentCache::LoadBinaryCache() {
  auto fp = cache_path_.Sub("jk.cache.binary");
  std::ifstream ifs(fp.Stringify());
  std::string line;

  while (std::getline(ifs, line)) {
    std::vector<std::string> sep;
    utils::SplitString(line, std::back_inserter(sep), ',');
    if (sep.size() != 2) {
      logger->warn("Parse binary cache failed. Ignore it!");
      caching_slots_.clear();
      return;
    }
    auto it = BinaryCreators.find(sep[0]);
    if (it == BinaryCreators.end()) {
      throw JKBuildError("TypeName {} not registered", sep[0]);
    }
    auto binary = utils::Base64Decode(sep[1]);
    auto slot = (it->second)(binary.data(), binary.size());
    caching_slots_[slot->Key()] = std::move(slot);
  }
}

}  // namespace jk::core::cache

// vim: fdm=marker
