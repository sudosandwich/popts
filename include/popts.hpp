#pragma once
#ifndef POPTS_OPTS_H_INCLUDED
#define POPTS_OPTS_H_INCLUDED

#pragma once
#ifndef POPTS_OPT_H_INCLUDED
#define POPTS_OPT_H_INCLUDED

#pragma once
#ifndef POPTS_TYPEDEFS_H_INCLUDED
#define POPTS_TYPEDEFS_H_INCLUDED

#include <chrono>
#include <deque>
#include <string>
#include <vector>

namespace popts {

using namespace std::string_literals;

using std::deque;
using std::string;
using std::vector;

using duration_t = std::chrono::duration<long double>;

} // namespace popts

#endif

namespace popts {

struct Option {
  using argv_t = vector<string>;

  static constexpr size_t Single = 1;
  static constexpr size_t Many = std::numeric_limits<size_t>::max();

  vector<string> m_names;
  string m_description;
  string m_defaultString;
  size_t m_count;
  bool m_isFlag;
  deque<argv_t::const_iterator> m_matches;
  deque<argv_t::const_iterator> m_parseErrors;

protected:
  unsigned int ParseMatches(const argv_t &argv);
};

template <typename T> struct OptionImpl : public Option {
  static T FlagMatchValue();
  static bool FromString(const std::string &data, T &out);
  static std::string ToString(const T &data);

  void ParseArguments(const argv_t &argv);

  deque<T> m_storage;
  T m_defaultArgument;
};

} // namespace popts

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

#endif

namespace popts {

class Options {
public:
  using argv_t = vector<string>;

private:
  struct tail_t {
    argv_t::const_iterator cbegin() const { return m_cbegin; }
    argv_t::const_iterator cend() const { return m_cend; }
    argv_t::const_iterator m_cbegin, m_cend;
  };

public:
  Options(const argv_t &argv);
  Options(int argc, char **argv);

  Options &WithHelp();

  bool HasDuplicateNames(std::ostream *out = nullptr) const;
  bool HasErrorMatches(std::ostream *out = nullptr) const;
  bool HasConsistentTail(std::ostream *out = nullptr) const;
  tail_t Tail() const;
  string Description() const;

  template <typename T>
  const T &MakeOption(std::initializer_list<const char *> names,
                      const T &defaultArgument, const string &description);

  template <typename T>
  const deque<T> &MakeOptions(std::initializer_list<const char *> names,
                              const string &description);

  const bool &Flag(std::initializer_list<const char *> names,
                   const string &description);

  const deque<bool> &Flags(std::initializer_list<const char *> names,
                           const string &description);

#define DEFINE_OPTION_FUNC(Type, Name)                                         \
  const Type &Options::Name(std::initializer_list<const char *> names,         \
                            const Type &defaultArgument,                       \
                            const string &description) {                       \
    return MakeOption<Type>(names, defaultArgument, description);              \
  }                                                                            \
                                                                               \
  const deque<Type> &Options::Name##s(                                         \
      std::initializer_list<const char *> names, const string &description) {  \
    return MakeOptions<Type>(names, description);                              \
  }

  DEFINE_OPTION_FUNC(string, String)
  DEFINE_OPTION_FUNC(bool, Bool)
  DEFINE_OPTION_FUNC(int64_t, Int)
  DEFINE_OPTION_FUNC(long double, Double)
  DEFINE_OPTION_FUNC(duration_t, Duration)

#undef DEFINE_OPTION_FUNC

private:
  template <typename T>
  OptionImpl<T> &AddOption(std::initializer_list<const char *> names,
                           const T &defaultArgument, const string &description,
                           size_t count, bool isFlag);

private:
  argv_t m_argv;
  argv_t::const_iterator m_tail = m_argv.cbegin();
  deque<std::unique_ptr<Option>> m_options;
};

} // namespace popts

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iosfwd>

namespace popts {

Options::Options(int argc, char **argv) : m_argv(argv, argv + argc) {}

Options::Options(const argv_t &argv) : m_argv(argv) {}

bool Options::HasDuplicateNames(std::ostream *out) const {
  vector<string> names;

  for (const auto &option : m_options) {
    std::copy(option->m_names.cbegin(), option->m_names.cend(),
              std::back_inserter(names));
  }

  std::sort(names.begin(), names.end());

  auto currentName = names.begin();
  auto end = names.end();
  bool hasDuplicates = false;

  while (currentName != end) {
    auto prevName = currentName++;
    if (currentName == end) {
      break;
    }

    if (*prevName == *currentName) {
      hasDuplicates = true;

      if (out) {
        (*out) << "Duplicate name: " << *currentName << "\n";
      } else {
        break;
      }

      // fast-forward through the duplicates
      currentName =
          std::find_if(currentName, end, [prevName](const auto &name) {
            return name != *prevName;
          });
    }
  }

  return hasDuplicates;
}

bool Options::HasErrorMatches(std::ostream *out) const {
  auto quotedArgument = [this](argv_t::const_iterator it) {
    if (it == m_argv.cend()) {
      return "<null>"s;
    }
    return "'"s + *it + "'"s;
  };

  auto commentSeparatedList = [this, quotedArgument](auto *out,
                                                     const auto &container) {
    auto from = std::cbegin(container);
    auto to = std::cend(container);

    if (from == to) {
      return;
    }

    (*out) << quotedArgument(*from);

    for (++from; from != to; ++from) {
      (*out) << ", " << quotedArgument(*from);
    }
  };

  bool hasErrors = false;
  vector<argv_t::const_iterator> allMatches;

  for (const std::unique_ptr<Option> &option : m_options) {
    // Check for errors
    if (!option->m_parseErrors.empty()) {
      hasErrors = true;

      if (!out) {
        break;
      }
      (*out) << "error matches for option '" << option->m_names[0] << "': ";
      commentSeparatedList(out, option->m_parseErrors);
      (*out) << "\n";
    }

    // Check if only one is allowed
    if (option->m_count == Option::Single && option->m_matches.size() > 1) {
      hasErrors = true;
      if (!out) {
        break;
      }

      (*out) << "multiple matches for single option '" << option->m_names[0]
             << "'";

      if (!option->m_isFlag) {
        (*out) << ": ";
        commentSeparatedList(out, option->m_matches);
      }

      (*out) << "\n";
    }

    // Check if match has been used as a argument value
    for (auto match : option->m_matches) {
      if (!option->m_isFlag) {
        allMatches.push_back(match);
      }
      allMatches.push_back(--match);
    }
  }

  std::sort(allMatches.begin(), allMatches.end());
  auto duplicateIt = std::adjacent_find(allMatches.cbegin(), allMatches.cend());
  while (duplicateIt != allMatches.cend()) {
    hasErrors = true;
    if (!out) {
      break;
    }

    (*out) << "Name consumed as argument before: '" << **duplicateIt << "'\n";

    duplicateIt = std::adjacent_find(++duplicateIt, allMatches.cend());
  }

  return hasErrors;
}

bool Options::HasConsistentTail(std::ostream *out) const {
  vector<argv_t::const_iterator> allConsumed;
  for (const auto &option : m_options) {
    for (auto it = option->m_matches.cbegin(); it != option->m_matches.cend();
         ++it) {
      allConsumed.push_back(std::prev(*it));
      if (!option->m_isFlag) {
        assert(*it != m_argv.cend());
        allConsumed.push_back(*it);
      }
    }
  }

  if (allConsumed.size() == 0) {
    return true;
  }

  // remove duplicates
  std::sort(allConsumed.begin(), allConsumed.end());
  allConsumed.erase(std::unique(allConsumed.begin(), allConsumed.end()),
                    allConsumed.end());

  bool hasHoles = false;
  // a slightly modified adjacent_find
  auto it = allConsumed.cbegin();
  auto end = allConsumed.cend();
  auto nextArg = std::next(it);

  for (; nextArg != end; ++nextArg, ++it) {
    auto nextArgv = std::next(*it);
    if (nextArgv == m_argv.cend()) {
      break;
    }

    if (*nextArg != nextArgv) {
      hasHoles = true;

      if (!out) {
        break;
      }

      while (nextArgv != *nextArg && nextArgv != m_argv.cend()) {
        (*out) << "unparsed argument '" << *nextArgv++ << "' before parsed '";
        (*out) << **nextArg << "'\n";
      }
    }
  }

  return !hasHoles;
}

Options::tail_t Options::Tail() const { return tail_t{m_tail, m_argv.cend()}; }

string Options::Description() const {
  vector<string> namesAndDefaults;
  namesAndDefaults.reserve(m_options.size());

  std::stringstream ss;
  for (const auto &option : m_options) {
    auto nameIt = std::cbegin(option->m_names);

    ss << *nameIt++;

    for (; nameIt != std::cend(option->m_names); ++nameIt) {
      ss << ", " << *nameIt;
    }

    if (option->m_count > Option::Single) {
      ss << " (...)";
    }

    if (!option->m_isFlag == option->m_count == Option::Single) {
      ss << " [=" << option->m_defaultString << "]";
    }

    namesAndDefaults.push_back(ss.str());
    ss.str(""s);
  }

  size_t slashPos = m_argv[0].find_last_of("/\\");
  const string &cmdName = (slashPos == string::npos)
                              ? (m_argv[0])
                              : (m_argv[0].data() + slashPos + 1);

  ss << "Usage '" << cmdName << "' [options]\n";

  const size_t colWidth =
      std::max_element(namesAndDefaults.cbegin(), namesAndDefaults.cend(),
                       [](const string &lhs, const string &rhs) {
                         return lhs.size() < rhs.size();
                       })
          ->size();

  auto nameIt = namesAndDefaults.cbegin();
  for (const auto &option : m_options) {
    ss << std::left << std::setw(colWidth + 4) << *nameIt++
       << option->m_description << "\n";
  }

  return ss.str();
}

template <typename T>
const T &Options::MakeOption(std::initializer_list<const char *> names,
                             const T &defaultArgument,
                             const string &description) {
  auto &option =
      AddOption(names, defaultArgument, description, Option::Single, false);
  return option.m_storage.front();
}

template <typename T>
const deque<T> &Options::MakeOptions(std::initializer_list<const char *> names,
                                     const string &description) {
  auto &option = AddOption(names, T(), description, Option::Many, false);
  return option.m_storage;
}

const bool &Options::Flag(std::initializer_list<const char *> names,
                          const string &description) {
  auto &option = AddOption(names, false, description, Option::Single, true);
  return option.m_storage.front();
}

const deque<bool> &Options::Flags(std::initializer_list<const char *> names,
                                  const string &description) {
  auto &option = AddOption(names, false, description, Option::Many, true);
  return option.m_storage;
}

template <typename T>
OptionImpl<T> &Options::AddOption(std::initializer_list<const char *> names,
                                  const T &defaultArgument,
                                  const string &description, size_t count,
                                  bool isFlag) {
  m_options.push_back(std::make_unique<OptionImpl<T>>());

  auto &option = static_cast<OptionImpl<T> &>(*m_options.back());
  std::copy(std::cbegin(names), std::cend(names),
            std::back_inserter(option.m_names));
  option.m_count = count;
  option.m_isFlag = isFlag;
  option.m_defaultArgument = defaultArgument;
  option.m_defaultString = OptionImpl<T>::ToString(defaultArgument);
  option.m_description = description;

  option.ParseArguments(m_argv);

  if (option.m_matches.size() > 0) {
    m_tail = std::max(std::next(option.m_matches.back()), m_tail);
  }

  assert(!HasDuplicateNames());

  return option;
}

} // namespace popts

#endif
