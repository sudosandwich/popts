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
