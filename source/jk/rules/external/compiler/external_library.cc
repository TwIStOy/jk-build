// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/compiler/external_library.hh"

#include <algorithm>
#include <iterator>
#include <vector>

#include "jk/common/counter.hh"
#include "jk/common/path.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/output/makefile.hh"
#include "jk/rules/external/rules/external_library.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::rules::external {

using fmt::operator"" _format;
using fmt::operator"" _a;

MakefileExternalLibraryCompiler::Result
MakefileExternalLibraryCompiler::DownloadAndDecompress(
    core::filesystem::ProjectFileSystem *project,
    core::output::UnixMakefile *makefile, ExternalLibrary *rule,
    const common::AbsolutePath &working_folder) const {
  auto output_folder = working_folder.Sub("decompressed_folder");

  makefile->DefineCommon(project);

  // jk download [URL] [OUTPUT] [SHA256]
  makefile->DefineEnvironment("DOWNLOAD", "jk download");
  makefile->DefineEnvironment("URL", rule->Url);
  std::vector<std::string> decompress_tpl = {};
  if (rule->ArchiveType == "tar.gz") {
    decompress_tpl = {"tar", "-xf", "{archive_file}", "-C", "{output_folder}"};
  } else {
    JK_THROW(core::JKBuildError("ArchiveType '{}' not supported.",
                                rule->ArchiveType));
  }

  auto download_print_idx = rule->KeyNumber("download");
  auto decompress_print_idx = rule->KeyNumber("decompress");
  // download file
  auto download_target = working_folder.Sub("DOWNLOAD").Stringify();
  {
    auto print_stmt = core::builder::CustomCommandLine::Make({
        "@$(PRINT)",
        "--switch=$(COLOR)",
        "--green",
        "--bold",
        "--progress-num={}"_format(download_print_idx),
        "--progress-dir={}"_format(project->BuildRoot),
        "Downloading {}"_format(working_folder.Sub(rule->OutputFile)),
    });
    auto download_stmt = core::builder::CustomCommandLine::Make(
        {"@$(DOWNLOAD)", "$(URL)",
         working_folder.Sub(rule->OutputFile).Stringify(), rule->Sha256,
         "$COLUMNS"});
    auto download_touch_stmt =
        core::builder::CustomCommandLine::Make({"@touch", download_target});
    makefile->AddTarget(download_target, {},
                        core::builder::CustomCommandLines::Multiple(
                            print_stmt, download_stmt, download_touch_stmt));
  }

  // decompress file
  auto decompress_target = working_folder.Sub("DECOMPRESS").Stringify();
  {
    auto print_stmt = core::builder::CustomCommandLine::Make({
        "@$(PRINT)",
        "--switch=$(COLOR)",
        "--green",
        "--bold",
        "--progress-num={},{}"_format(download_print_idx, decompress_print_idx),
        "--progress-dir={}"_format(project->BuildRoot),
        "Decompressing {}"_format(working_folder.Sub(rule->OutputFile)),
    });
    std::vector<std::string> decompress_stmt;
    std::transform(
        std::begin(decompress_tpl), std::end(decompress_tpl),
        std::back_inserter(decompress_stmt), [&](const std::string &tpl) {
          return fmt::format(
              tpl, "archive_file"_a = working_folder.Sub(rule->OutputFile),
              "output_folder"_a = output_folder.Stringify());
        });
    auto touch_stmt =
        core::builder::CustomCommandLine::Make({"@touch", decompress_target});
    makefile->AddTarget(
        decompress_target, {download_target},
        core::builder::CustomCommandLines::Multiple(
            print_stmt,
            core::builder::CustomCommandLine::FromVec(decompress_stmt),
            touch_stmt));
  }

  return {output_folder,
          {download_print_idx, decompress_print_idx},
          download_target,
          decompress_target};
}

}  // namespace jk::rules::external

// vim: fdm=marker

