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
