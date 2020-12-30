// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <unordered_map>
#include <vector>

#include "boost/multi_index/hashed_index.hpp"
#include "boost/multi_index/mem_fun.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index_container.hpp"
#include "jk/common/path.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/rules/cc/source_file.hh"

namespace jk::rules::cc {

class SourceFileRecorder {
 public:
  static SourceFileRecorder *Instance();

  bool PathExists(const std::string &p) const;

  void Push(std::string p, BuildRule *rule);

  BuildRule *Rule(const std::string &p) const;

 private:
  SourceFileRecorder() = default;

  struct SourceFileSlot {
    std::string Path;
    BuildRule *Rule;

    std::string RuleName() const;
  };

  boost::multi_index::multi_index_container<
      SourceFileSlot,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_non_unique<
              boost::multi_index::const_mem_fun<SourceFileSlot, std::string,
                                                &SourceFileSlot::RuleName>>,
          boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(
              SourceFileSlot, std::string, Path)>>>
      mp_;
};

}  // namespace jk::rules::cc

// vim: fdm=marker
