#include <algorithm>
#include <cctype>   //std::tolower
#include <charconv> //std::from_chars
#include <regex>
#include <sstream>

namespace popts {

unsigned int Option::ParseMatches(const argv_t &argv) {
  auto from = std::next(argv.cbegin());
  auto to = argv.cend();

  for (auto match = from;;) {
    match = std::find_first_of(match, to, m_names.cbegin(), m_names.cend());

    if (match == to) {
      break;
    };

    m_matches.push_back(++match);
  }

  return m_matches.size();
}

template <typename T>
// static
bool OptionImpl<T>::FromString(const std::string &data, T &out) {
  std::stringstream ss;
  ss << data;
  ss >> out;
  return !ss.fail();
}

template <>
// static
bool OptionImpl<duration_t>::FromString(const std::string &data,
                                        duration_t &out) {
  // since C++20 is not out yet...
  std::regex durationRe("(\\d+)(d|h|m|s|ms|ns)"s);
  std::smatch m;
  if (!std::regex_match(data, m, durationRe)) {
    return false;
  }

  std::string numbers = m[1].str();
  std::string unit = m[2].str();

  long double value;
  std::from_chars(numbers.data(), numbers.data() + numbers.size(), value);

  if (unit == "d"s) {
    out = std::chrono::duration<long double, std::ratio<86400>>(value);
  } else if (unit == "h"s) {
    out = std::chrono::duration<long double, std::ratio<3600>>(value);
  } else if (unit == "m"s) {
    out = std::chrono::duration<long double, std::ratio<60>>(value);
  } else if (unit == "s"s) {
    out = std::chrono::duration<long double, std::ratio<1>>(value);
  } else if (unit == "ms"s) {
    out = std::chrono::duration<long double, std::milli>(value);
  } else if (unit == "ns"s) {
    out = std::chrono::duration<long double, std::nano>(value);
  } else {
    return false;
  }

  return true;
}

template <>
// static
bool OptionImpl<std::string>::FromString(const std::string &data,
                                         std::string &out) {
  out = data;
  return true;
}

template <>
// static
bool OptionImpl<bool>::FromString(const std::string &data, bool &out) {
  static const auto truthy = {"true"s, "1"s, "on"s, "yes"s, "y"s};
  static const auto falsy = {"false"s, "0"s, "off"s, "no"s, "n"s};

  std::string lowerdata;
  lowerdata.reserve(data.size());

  std::transform(data.cbegin(), data.cend(), std::back_inserter(lowerdata),
                 [](int C) { return ::std::tolower(C); });

  if (std::find(std::cbegin(truthy), std::cend(truthy), lowerdata) !=
      std::cend(truthy)) {
    out = true;
    return true;
  } else if (std::find(std::cbegin(falsy), std::cend(falsy), lowerdata) !=
             std::cend(falsy)) {
    out = false;
    return true;
  } else {
    out = true;
    return false;
  }
}

template <typename T>
// static
std::string OptionImpl<T>::ToString(const T &data) {
  std::stringstream ss;
  ss << data;
  return ss.str();
}

template <>
// static
std::string OptionImpl<duration_t>::ToString(const duration_t &data) {
  std::stringstream ss;
  ss << data.count() << "s";
  return ss.str();
}

template <typename T> void OptionImpl<T>::ParseArguments(const argv_t &argv) {
  ParseMatches(argv);

  m_storage.clear();
  m_parseErrors.clear();

  if (m_isFlag) {
    std::fill_n(std::back_inserter(m_storage), m_matches.size(),
                FlagMatchValue());
  } else {
    for (auto match : m_matches) {
      if (match != argv.cend()) {
        T value;
        if (FromString(*match, value)) {
          m_storage.push_back(value);
        } else {
          m_parseErrors.push_back(match);
        }
      } else {
        m_parseErrors.push_back(match);
      }
    }
  }

  if (m_count == Single) {
    m_storage.push_back(m_defaultArgument);
  }
}

template <typename T> T OptionImpl<T>::FlagMatchValue() { return T(); }

template <> bool OptionImpl<bool>::FlagMatchValue() { return true; }

} // namespace popts
