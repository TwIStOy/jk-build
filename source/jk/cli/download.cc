// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/download.hh"

#include <curl/curl.h>
#include <openssl/sha.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "args.hxx"
#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/bytes.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/progress.hh"
#include "jk/utils/str.hh"

namespace jk::cli {

static std::optional<std::string> read_proxy_settings() {
  auto http_proxy = getenv("HTTP_PROXY");
  if (http_proxy) {
    return http_proxy;
  }
  return {};
}

static std::string sha256_hash_string(
    unsigned char hash[SHA256_DIGEST_LENGTH]) {
  std::ostringstream oss;
  for (auto i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    oss << "{:02x}"_format(hash[i]);
  }
  return oss.str();
}

static std::string HashFile(const common::AbsolutePath &file,
                            const std::string &method) {
  if (method == "sha256") {
    FILE *fp = fopen(file.Stringify().c_str(), "rb");
    if (!fp) {
      JK_THROW(core::JKBuildError("Open file {} error.", file));
      return "";
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    uint8_t *buffer   = static_cast<uint8_t *>(malloc(bufSize));
    int bytesRead     = 0;
    if (!buffer) {
      JK_THROW(core::JKBuildError("Not enough memory"));
    }
    while ((bytesRead = fread(buffer, 1, bufSize, fp))) {
      SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    auto res = sha256_hash_string(hash);

    fclose(fp);
    free(buffer);
    return res;
  }

  JK_THROW(core::JKBuildError("Not supported."));
}

class CURLEasyGuard {
 public:
  explicit CURLEasyGuard(CURL *easy) : Easy(easy) {
  }

  ~CURLEasyGuard() {
    if (this->Easy) {
      ::curl_easy_cleanup(this->Easy);
    }
  }

  CURLEasyGuard(const CURLEasyGuard &)            = delete;
  CURLEasyGuard &operator=(const CURLEasyGuard &) = delete;

  void release() {
    this->Easy = nullptr;
  }

 private:
  ::CURL *Easy;
};

static void Fail(const std::string &stage, const std::string &msg) {
  JK_THROW(core::JKBuildError("{}: {}", stage, msg));
}

static void CheckCurlResult(::CURLcode result, const std::string &msg) {
  if (result != CURLE_OK) {
    Fail(msg, ::curl_easy_strerror(result));
  }
}

static size_t WriteToFileCallback(void *ptr, size_t size, size_t nmemb,
                                  void *data) {
  int realsize        = static_cast<int>(size * nmemb);
  std::ofstream *fout = static_cast<std::ofstream *>(data);
  const char *chPtr   = static_cast<char *>(ptr);
  fout->write(chPtr, realsize);
  return realsize;
}

static size_t FileCommandCurlDebugCallback(::CURL *, curl_infotype type,
                                           char *chPtr, size_t size,
                                           void *data) {
  switch (type) {
    case CURLINFO_TEXT:
    case CURLINFO_HEADER_IN:
    case CURLINFO_HEADER_OUT:
      break;
    case CURLINFO_DATA_IN:
    case CURLINFO_DATA_OUT:
    case CURLINFO_SSL_DATA_IN:
    case CURLINFO_SSL_DATA_OUT: {
      std::cout << "[%{}bytes data]"_format(size) << std::endl;
    } break;
    default:
      break;
  }
  return 0;
}

static curl_off_t previous_dl = 0;
static std::chrono::time_point<std::chrono::high_resolution_clock> start_ts =
    std::chrono::high_resolution_clock::now();

static int FileDownloadProgressCallback(void *clientp, curl_off_t dltotal,
                                        curl_off_t dlnow, curl_off_t ultotal,
                                        curl_off_t ulnow) {
  utils::ProgressBar *helper = reinterpret_cast<utils::ProgressBar *>(clientp);

  static_cast<void>(ultotal);
  static_cast<void>(ulnow);

  auto ts    = std::chrono::high_resolution_clock::now();
  auto delta = dlnow - previous_dl;
  if (delta == 0) {
    return 0;
  }

  std::chrono::duration<double> diff = ts - start_ts;

  previous_dl = dlnow;

  auto speed = dlnow / diff.count();

  auto msg =
      "{}/{} {}/s"_format(utils::BytesCount(dlnow), utils::BytesCount(dltotal),
                          utils::BytesCount(speed));

  helper->Print(std::cout, dlnow, dltotal, msg);

  return 0;
}

static void Download(const std::string &url, const common::AbsolutePath &output,
                     const std::string &sha256, bool verbose, int timeout,
                     int inactivity_timeout, uint32_t column_size) {
  if (fs::exists(output.Path) &&
      utils::EqualIgnoreCase(HashFile(output, "sha256"), sha256)) {
    std::cout << "File exists" << std::endl;
    return;
  }
  common::AssumeFolder(output.Path.parent_path());

  // std::cout << "Start download..." << std::endl;

  ::CURL *curl;
  ::curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = ::curl_easy_init();
  if (!curl) {
    Fail("init", "Error in initializing curl.");
  }

  CURLEasyGuard guard(curl);
  ::CURLcode res = ::curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  CheckCurlResult(res, "DOWNLOAD cannot set url");

  // enable HTTP ERROR parsing
  res = ::curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  CheckCurlResult(res, "DOWNLOAD cannot set http failure option");

  res = ::curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/" LIBCURL_VERSION);
  CheckCurlResult(res, "DOWNLOAD cannot set user agent option");

  res = ::curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFileCallback);
  CheckCurlResult(res, "DOWNLOAD cannot set write function");

  res = ::curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION,
                           FileCommandCurlDebugCallback);
  CheckCurlResult(res, "DOWNLOAD cannot set debug function");

  std::ofstream fout(output.Stringify(), std::ios::out | std::ios::binary);

  res = ::curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fout);
  CheckCurlResult(res, "DOWNLOAD cannot set write data");

  res = ::curl_easy_setopt(curl, CURLOPT_DEBUGDATA, nullptr);
  CheckCurlResult(res, "DOWNLOAD cannot set debug data");

  res = ::curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  CheckCurlResult(res, "DOWNLOAD cannot set follow-redirect option");

  if (verbose) {
    res = ::curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    CheckCurlResult(res, "DOWNLOAD cannot set verbose: ");
  }

  if (timeout > 0) {
    res = ::curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    CheckCurlResult(res, "DOWNLOAD cannot set timeout: ");
  }

  if (inactivity_timeout > 0) {
    // Give up if there is no progress for a long time.
    ::curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1);
    ::curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, inactivity_timeout);
  }

  if (auto proxy = read_proxy_settings(); proxy) {
    ::curl_easy_setopt(curl, CURLOPT_PROXY, proxy.value().c_str());
    CheckCurlResult(res, "DOWNLOAD catnot set proxy value");
  }

  utils::ProgressBar bar(column_size);

  res = ::curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
  CheckCurlResult(res, "DOWNLOAD cannot set noprogress value");

  res = ::curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
                           FileDownloadProgressCallback);
  CheckCurlResult(res, "DOWNLOAD cannot set progress function");

  res = ::curl_easy_setopt(curl, CURLOPT_XFERINFODATA,
                           reinterpret_cast<void *>(&bar));
  CheckCurlResult(res, "DOWNLOAD cannot set progress data");

  start_ts = std::chrono::high_resolution_clock::now();
  res      = ::curl_easy_perform(curl);
  guard.release();
  ::curl_easy_cleanup(curl);

  ::curl_global_cleanup();
  fout.flush();
  fout.close();

  auto actual = HashFile(output, "sha256");
  if (!utils::EqualIgnoreCase(actual, sha256)) {
    JK_THROW(core::JKBuildError(
        "Sha256 mismatch, for file: {}, expect: {}, actual: {}, status: {}",
        output, sha256, actual, ::curl_easy_strerror(res)));
  }
}

void DownloadFile(args::Subparser &parser) {
  args::PositionalList<std::string> positional(parser, "pos", "...");
  parser.Parse();

  std::vector<std::string> args{std::begin(positional), std::end(positional)};
  if (args.size() < 4) {
    JK_THROW(core::JKBuildError("Not enough arguments"));
    return;
  }

  uint32_t column_size = std::atoi(args[3].c_str());

  Download(args[0], common::AbsolutePath(fs::path{args[1]}), args[2], false, 0,
           0, column_size);
}

}  // namespace jk::cli

// vim: fdm=marker
