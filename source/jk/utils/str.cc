// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/str.hh"

#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace jk::utils {

bool StringEndsWith(std::string_view full_string,  // {{{
                    std::string_view ending) {
  if (full_string.length() >= ending.length()) {
    return (0 == full_string.compare(full_string.length() - ending.length(),
                                     ending.length(), ending));
  } else {
    return false;
  }
}  // }}}

bool StringStartsWith(std::string_view full_string,  // {{{
                      std::string_view prefix) {
  if (full_string.length() >= prefix.length()) {
    return full_string.substr(prefix.length()) == prefix;
  } else {
    return false;
  }
}  // }}}

std::string ToLower(const std::string &s) {  // {{{
  std::string res;
  std::transform(std::begin(s), std::end(s), std::back_inserter(res),
                 [](char ch) {
                   return std::tolower(ch);
                 });
  return res;
}  // }}}

bool EqualIgnoreCase(std::string_view lhs, std::string_view rhs) {  // {{{
  constexpr auto CNT = 'A' - 'a';
  if (lhs.length() != rhs.length()) {
    return false;
  }
  for (auto i = 0u; i < lhs.length(); ++i) {
    if (lhs[i] == rhs[i]) {
      continue;
    }
    if (std::tolower(lhs[i]) == std::tolower(rhs[i])) {
      continue;
    }
    return true;
  }
  return false;
}  // }}}

void ReplaceAllSlow(std::string *text, std::string_view from,  // {{{
                    std::string_view to, int count) {
  if (from.empty())
    return;

  size_t start_pos = 0;
  while ((start_pos = text->find(from, start_pos)) != std::string::npos &&
         (count < 0 || count > 0)) {
    text->replace(start_pos, from.length(), to);
    start_pos += to.length();
    count--;
  }
}  // }}}

std::string EscapeForShellStyle(std::string_view raw) {  // {{{
  std::string result;
  for (const char *ch = raw.data(); *ch != '\0'; ++ch) {
    if (*ch == ' ' || *ch == '<' || *ch == '>') {
      result += '\\';
    }
    result += *ch;
  }
  return result;
}  // }}}

static const std::string_view base64_chars =  // {{{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";  // }}}

static inline bool is_base64(unsigned char c) {  // {{{
  return (isalnum(c) || (c == '+') || (c == '/'));
}  // }}}

std::string Base64Encode(uint8_t *str, uint32_t len) {  // {{{
  std::string ret;
  int i = 0;
  int j = 0;
  uint8_t char_array_3[3];  // store 3 byte of bytes_to_encode
  uint8_t char_array_4[4];  // store encoded character to 4 bytes

  while (len--) {
    char_array_3[i++] = *(str++);  // get three bytes (24 bits)
    if (i == 3) {
      // eg. we have 3 bytes as ( 0100 1101, 0110 0001, 0110 1110) --> (010011,
      // 010110, 000101, 101110)
      char_array_4[0] =
          (char_array_3[0] & 0xfc) >> 2;  // get first 6 bits of first byte,
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) +
          ((char_array_3[1] & 0xf0) >>
           4);  // get last 2 bits of first byte and first 4 bit of second byte
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) +
          ((char_array_3[2] & 0xc0) >>
           6);  // get last 4 bits of second byte and first 2 bits of third byte
      char_array_4[3] =
          char_array_3[2] & 0x3f;  // get last 6 bits of third byte

      for (i = 0; (i < 4); i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while ((i++ < 3))
      ret += '=';
  }

  return ret;
}  // }}}

std::vector<uint8_t> Base64Decode(std::string_view str) {  // {{{
  auto in_len = str.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::vector<uint8_t> ret;

  while (in_len-- && (str[in_] != '=') && is_base64(str[in_])) {
    char_array_4[i++] = str[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = 0; j < i; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

    for (j = 0; (j < i - 1); j++)
      ret.push_back(char_array_3[j]);
  }

  return ret;
}  // }}}

std::string RandomAlphaNumString(uint32_t length) {  // {{{
  static std::string_view choices =
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789";
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<int> dis(0, choices.size() - 1);

  std::string res;
  while (length--) {
    res.push_back(choices[dis(gen)]);
  }
  return res;
}  // }}}

}  // namespace jk::utils

// vim: fdm=marker
