// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/models/cc/source_file.hh"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/build_rule.hh"

namespace jk::impls::models::cc {

static absl::flat_hash_set<std::string> CppExtensions = {
    ".cc",
    ".cpp",
    ".cxx",
};

static absl::flat_hash_set<std::string> CExtensions = {
    ".c",
};

static absl::flat_hash_set<std::string> HeaderExtensions = {".h", ".hh", ".hxx",
                                                            ".hpp"};

SourceFile::SourceFile(const common::ProjectRelativePath &filename,
                       core::models::BuildRule *rule)
    : Rule(rule),
      FullQualifiedPath(filename),
      FullQualifiedObjectPath([this]() {
        auto p = FullQualifiedPath;
        p.Path = p.Path.parent_path() / (p.Path.filename().string() + ".o");
        return p;
      }()) {
  IsCSourceFile = [this]() {
    auto ext =
        absl::AsciiStrToLower(FullQualifiedPath.Path.extension().string());
    return CExtensions.contains(ext);
  }();

  IsCppSourceFile = [this]() {
    auto ext =
        absl::AsciiStrToLower(FullQualifiedPath.Path.extension().string());
    return CppExtensions.contains(ext);
  }();

  IsSourceFile = [this]() {
    return IsCSourceFile || IsCppSourceFile;
  }();

  IsHeaderFile = [this]() {
    auto ext =
        absl::AsciiStrToLower(FullQualifiedPath.Path.extension().string());
    return HeaderExtensions.contains(ext);
  }();
}

SourceFile::SourceFile(std::string filename, core::models::BuildRule *rule)
    : Rule(rule),
      FullQualifiedPath(Rule->Package->Path.Sub(filename)),
      FullQualifiedObjectPath([this]() {
        auto p = FullQualifiedPath;
        p.Path = p.Path.parent_path() / (p.Path.filename().string() + ".o");
        return p;
      }()) {
  IsCSourceFile = [this]() {
    auto ext =
        absl::AsciiStrToLower(FullQualifiedPath.Path.extension().string());
    return CExtensions.contains(ext);
  }();

  IsCppSourceFile = [this]() {
    auto ext =
        absl::AsciiStrToLower(FullQualifiedPath.Path.extension().string());
    return CppExtensions.contains(ext);
  }();

  IsSourceFile = [this]() {
    return IsCSourceFile || IsCppSourceFile;
  }();

  IsHeaderFile = [this]() {
    auto ext =
        absl::AsciiStrToLower(FullQualifiedPath.Path.extension().string());
    return HeaderExtensions.contains(ext);
  }();
}

auto SourceFile::ResolveFullQualifiedPath(
    const common::AbsolutePath &new_root) const -> common::AbsolutePath {
  return new_root.Sub(FullQualifiedPath.Path);
}

common::AbsolutePath SourceFile::ResolveFullQualifiedObjectPath(
    const common::AbsolutePath &new_root, std::string_view build_type) const {
  return new_root.Sub(build_type, FullQualifiedObjectPath);
}

auto SourceFile::ResolveFullQualifiedDotDPath(
    const common::AbsolutePath &new_root) const -> common::AbsolutePath {
  auto p = FullQualifiedPath;
  p.Path = p.Path.parent_path() / (p.Path.filename().string() + ".d");
  return new_root.Sub(p.Path);
}

auto SourceFile::ResolveFullQualifiedLintPath(
    const common::AbsolutePath &new_root) const -> common::AbsolutePath {
  auto p = FullQualifiedPath;
  p.Path = p.Path.parent_path() / (p.Path.filename().string() + ".lint");
  return new_root.Sub(p.Path);
}

auto SourceFile::ResolveFullQualifiedPbPath(
    const common::AbsolutePath &new_root) const -> common::AbsolutePath {
  auto p        = FullQualifiedPath;
  auto filename = p.Path.filename().string();
  filename      = filename.substr(0, filename.find_last_of('.'));
  p.Path        = p.Path.parent_path() / (filename + ".pb");

  return new_root.Sub(p.Path);
}

}  // namespace jk::impls::models::cc
