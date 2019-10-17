#pragma once
#ifndef POPTS_OPTS_H_INCLUDED
#define POPTS_OPTS_H_INCLUDED

#include "opt.h"

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

#include "opts.inl.h"

#endif
