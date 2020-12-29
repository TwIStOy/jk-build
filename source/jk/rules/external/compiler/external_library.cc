// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/compiler/external_library.hh"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
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

std::string MakefileExternalLibraryCompiler::Name() const {
  return "Makefile.external_library";
}

void MakefileExternalLibraryCompiler::CompileImpl(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, ExternalLibrary *rule,
    std::function<void(core::output::UnixMakefile *)> prepare,
    const std::vector<std::string> &_download_command,
    const std::vector<std::string> &_configure_command,
    const std::vector<std::string> &_build_command,
    const std::vector<std::string> &_install_command) const {
  auto working_folder = rule->WorkingFolder(project->BuildRoot);

  auto makefile = std::make_unique<core::output::UnixMakefile>("build.make");
  makefile->DefineCommon(project);

  makefile->DefineEnvironment("URL", rule->Url);
  makefile->DefineEnvironment("SHA256", rule->Sha256);

  prepare(makefile.get());

  makefile->AddTarget("all", {"build"}, {}, "", true);

  auto TouchMarker = [](const auto &marker) {
    return core::builder::CustomCommandLine::Make({"@touch", marker});
  };
  auto PrintStmt = [project, rule](const std::string &key,
                                   const std::string &msg) {
    return core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
         "--progress-num={}"_format(rule->KeyNumber(key)),
         "--progress-dir={}"_format(project->BuildRoot), msg});
  };

  // try to update url and checksum
  common::AssumeFolder(working_folder.Path);
  common::FastWriteFile(working_folder.Sub("url-cache").Path, rule->Url);
  common::FastWriteFile(working_folder.Sub("checksum-cache").Path,
                        rule->Sha256);

  // stage 1: download   | marker: stage-download {{{
  auto output_filename = "{}.{}"_format(rule->Name, rule->ArchiveType);
  auto download_marker = working_folder.Sub("stage-download");
  auto download_print = PrintStmt(
      "download", "Downloading package from {} with checksum: {}"_format(
                      rule->Url, rule->Sha256));
  auto download_command = core::builder::CustomCommandLine{};
  if (_download_command.empty()) {
    // use default download command:
    // jk download [URL] [OUTPUT] [SHA256]
    download_command = core::builder::CustomCommandLine::Make(
        {"@$(JK_COMMAND)", "download", "$(URL)", output_filename, "$(SHA256)"});
  } else {
    download_command =
        core::builder::CustomCommandLine::FromVec(_download_command);
  }
  makefile->AddTarget(
      download_marker,
      {working_folder.Sub("url-cache"), working_folder.Sub("checksum-cache")},
      core::builder::CustomCommandLines::Multiple(
          download_print, download_command, TouchMarker(download_marker)));
  // }}}

  // stage 2: decompress | marker: stage-decompress {{{
  auto decompress_marker = working_folder.Sub("stage-decompress");
  auto decompress_print =
      PrintStmt("decompress", "Decompressing {}"_format(output_filename));
  auto output_folder = working_folder.Sub("decompressed_folder");

  makefile->DefineEnvironment("THIS_SOURCE_DIR", output_folder);

  std::vector<std::string> decompress_tpl = {};
  if (rule->ArchiveType == "tar.gz") {
    decompress_tpl = {"tar", "-xf", "{archive_file}", "-C", "{output_folder}"};
  } else {
    JK_THROW(core::JKBuildError("ArchiveType '{}' not supported.",
                                rule->ArchiveType));
  }
  std::vector<std::string> decompress_stmt;
  std::transform(
      std::begin(decompress_tpl), std::end(decompress_tpl),
      std::back_inserter(decompress_stmt), [&](const std::string &tpl) {
        return fmt::format(
            tpl, "archive_file"_a = working_folder.Sub(output_filename),
            "output_folder"_a = output_folder.Stringify());
      });
  makefile->AddTarget(
      decompress_marker, {download_marker},
      core::builder::CustomCommandLines::Multiple(
          decompress_print,
          core::builder::CustomCommandLine::FromVec(decompress_stmt),
          TouchMarker(decompress_marker)));
  // }}}

  // stage 3: configure  | marker: stage-configure {{{
  auto configure_marker = working_folder.Sub("stage-configure");
  auto configure_print_command = PrintStmt("configure", "Configuring...");
  auto configure_command = core::builder::CustomCommandLine{};
  if (_configure_command.empty()) {
    // use default configure command:
    // ./configure
    configure_command = core::builder::CustomCommandLine::Make(
        {"cd", output_folder, "&&", "./configure", "--prefix",
         project->ExternalInstalledPrefix()});
  } else {
    std::vector<std::string> base{"cd", output_folder, "&&"};
    std::copy(std::begin(_configure_command), std::end(_configure_command),
              std::back_inserter(base));

    configure_command = core::builder::CustomCommandLine::FromVec(base);
  }
  makefile->AddTarget(configure_marker, {decompress_marker},
                      core::builder::CustomCommandLines::Multiple(
                          configure_print_command, configure_command,
                          TouchMarker(configure_marker)));
  // }}}

  // stage 4: build      | marker: stage-build {{{
  auto build_marker = working_folder.Sub("stage-build");
  auto build_print_command = PrintStmt("build", "Building...");
  auto build_command = core::builder::CustomCommandLine{};
  if (_build_command.empty()) {
    build_command = core::builder::CustomCommandLine::Make(
        {"cd", output_folder, "&&", "$(MAKE)"});
  } else {
    build_command =
        core::builder::CustomCommandLine::Make({"cd", output_folder, "&&"});
    std::copy(std::begin(_build_command), std::end(_build_command),
              std::back_inserter(build_command));
  }
  makefile->AddTarget(
      build_marker, {configure_marker},
      core::builder::CustomCommandLines::Multiple(
          build_print_command, build_command, TouchMarker(build_marker)));
  // }}}

  // stage 5: install    | marker: stage-install {{{
  auto install_marker = working_folder.Sub("stage-install");
  auto install_print_command = PrintStmt("install", "Installing...");
  auto install_command = core::builder::CustomCommandLine{};
  if (_install_command.empty()) {
    install_command = core::builder::CustomCommandLine::Make(
        {"cd", output_folder, "&&", "$(MAKE)", "install"});
  } else {
    install_command =
        core::builder::CustomCommandLine::Make({"cd", output_folder, "&&"});
    std::copy(std::begin(_install_command), std::end(_install_command),
              std::back_inserter(install_command));
  }
  makefile->AddTarget(
      install_marker, {build_marker},
      core::builder::CustomCommandLines::Multiple(
          install_print_command, install_command, TouchMarker(install_marker)));
  // }}}

  // stage 6: complete   | marker: stage-complete {{{
  auto complete_marker = working_folder.Sub("stage-complete");
  auto complete_print_command = core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
       "--progress-num={}"_format(utils::JoinString(",", rule->KeyNumbers())),
       "--progress-dir={}"_format(project->BuildRoot),
       "External Library {} installed!"_format(rule->Name)});
  makefile->AddTarget(
      complete_marker, {install_marker},
      core::builder::CustomCommandLines::Multiple(
          complete_print_command, TouchMarker(complete_marker)));
  // }}}

  makefile->AddTarget("build", {complete_marker}, {}, "", true);

  auto w = wf->Build(working_folder.Sub("build.make"));
  makefile->Write(w.get());
}

void MakefileExternalLibraryCompiler::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<ExternalLibrary>();
  CompileImpl(
      project, wf, rule,
      [](auto) {
      },
      rule->DownloadCommand, rule->ConfigureCommand, rule->BuildCommand,
      rule->InstallCommand);
}

}  // namespace jk::rules::external

// vim: fdm=marker
