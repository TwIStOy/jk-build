// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/recorder.hh"

#include <string>
#include <utility>

#include "jk/rules/cc/source_file.hh"

namespace jk::rules::cc {

SourceFileRecorder *SourceFileRecorder::Instance() {
  static SourceFileRecorder recorder;
  return &recorder;
}

std::string SourceFileRecorder::SourceFileSlot::RuleName() const {
  return Rule->FullQualifiedName();
}

bool SourceFileRecorder::PathExists(const std::string &p) const {
  auto &indexer = mp_.get<0>();

  return indexer.find(p) != indexer.end();
}

void SourceFileRecorder::Push(std::string p, BuildRule *rule) {
  auto slot = SourceFileSlot{std::move(p), rule};
  mp_.insert(std::move(slot));
}

BuildRule *SourceFileRecorder::Rule(const std::string &p) const {
  auto &indexer = mp_.get<0>();

  if (auto it = indexer.find(p); it != indexer.end()) {
    return it->Rule;
  }
  return nullptr;
}

}  // namespace jk::rules::cc

// vim: fdm=marker
