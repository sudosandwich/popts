#pragma once
#ifndef POPTS_OPT_H_INCLUDED
#define POPTS_OPT_H_INCLUDED

#include "typedefs.h"

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

#include "opt.inl.h"

#endif
