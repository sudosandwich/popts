#include "opts.h"

#include "catch2/catch.hpp"

#include <complex>
#include <iostream>
#include <iterator>

///////////////////////////////////////////////////////////////////////////////
// Nomenclature
//
// ./cmd --name argument --flag positio nal
//
// command: 'cmd'
// option: '--name argument' or '--flag'
//   parameter: '--name argument'
//   flag: '--flag'
// name: '--name'
// argument: 'argument'
// tail: 'positio nal'
//

using namespace std;

TEST_CASE("Parse single strings", "[parser]") {
  popts::Options popts(
      vector<string>({"path/cmd", "-f", "fn", "--alt", "fn2"}));

  auto s1 = popts.String({"-f"}, "xx", "xx");
  auto s2 = popts.String({"-x", "--alt"}, "yy", "yy");

  REQUIRE(s1 == "fn"s);
  REQUIRE(s2 == "fn2"s);
}

TEST_CASE("Parse multiple strings", "[parser]") {
  popts::Options popts(vector<string>({"path/cmd", "-f", "fn", "-f", "fn2"}));

  auto s1 = popts.Strings({"-f"}, "xx");

  REQUIRE(s1.size() == 2);
  REQUIRE(s1[0] == "fn"s);
  REQUIRE(s1[1] == "fn2"s);
}

TEST_CASE("Parse flags", "[parser]") {
  popts::Options popts(vector<string>({"path/cmd", "-f", "-v", "-v", "-v"}));

  auto flag = popts.Flag({"-f"}, "flag");
  auto flags = popts.Flags({"-v"}, "flags");

  REQUIRE(flag == true);
  REQUIRE(flags.size() == 3);
}

TEST_CASE("Parse bools", "[parser]") {
  popts::Options popts(vector<string>(
      {"path/cmd", "-a", "1", "-b", "0", "-c", "tRuE", "-d", "yeS"}));

  auto a = popts.Bool({"-a"}, false, "");
  auto b = popts.Bool({"-b"}, true, "");
  auto c = popts.Bool({"-c"}, false, "");
  auto d = popts.Bool({"-d"}, false, "");
  auto e = popts.Bool({"-e"}, true, "");

  REQUIRE(a == true);
  REQUIRE(b == false);
  REQUIRE(c == true);
  REQUIRE(d == true);
  REQUIRE(e == true);
}

TEST_CASE("Parse durations", "[parser]") {
  popts::Options popts(
      vector<string>({"path/cmd", "-a", "42ns", "-b", "43ms", "-c", "44s", "-d",
                      "45m", "-e", "46h", "-f", "47d"}));

  auto ns = popts.Duration({"-a"}, std::chrono::nanoseconds(1), "");
  auto ms = popts.Duration({"-b"}, std::chrono::milliseconds(1), "");
  auto s = popts.Duration({"-c"}, std::chrono::seconds(1), "");
  auto m = popts.Duration({"-d"}, std::chrono::seconds(0), "");
  auto h = popts.Duration({"-e"}, std::chrono::seconds(0), "");
  auto d = popts.Duration({"-f"}, std::chrono::seconds(0), "");

  REQUIRE(ns == std::chrono::nanoseconds(42));
  REQUIRE(ms == std::chrono::milliseconds(43));
  REQUIRE(s == std::chrono::seconds(44));
  REQUIRE(m == std::chrono::minutes(45));
  REQUIRE(h == std::chrono::hours(46));
  REQUIRE(d == std::chrono::hours(24 * 47));
}

TEST_CASE("Parsing custom types (std::complex)", "[parser]") {
  popts::Options popts(vector<string>({"path/cmd", "-c", "(4,3)"}));

  using compl = complex<double>;
  auto c = popts.MakeOption<compl>({"-c"}, compl(), "complex");

  REQUIRE(c == compl(4, 3));
}

TEST_CASE("Duplicate definitions", "[errors]") {
  popts::Options popts(vector<string>({"path/cmd"}));

  SECTION("Duplicate flags") {
    popts.Flag({"--foo", "-f"}, "");
    REQUIRE(!popts.HasDuplicateNames());
#if defined(NDEBUG) || defined(_NDEBUG)
    popts.Flag({"-f", "--bar"}, "");
    REQUIRE(popts.HasDuplicateNames());
#endif
  }

  SECTION("Duplicate option") {
    popts.Bool({"-g", "--goo"}, true, "");
    REQUIRE(!popts.HasDuplicateNames());
#if defined(NDEBUG) || defined(_NDEBUG)
    popts.Bool({"--goo", "-h"}, true, "");
    REQUIRE(popts.HasDuplicateNames());
#endif
  }
}

TEST_CASE("Duplicate matches", "[errors]") {
  popts::Options popts(
      vector<string>({"path/cmd", "-f", "-f", "-o", "x", "-o", "y", "-u"}));

  SECTION("Flags") {
    auto flag = popts.Flag({"--foo", "-f"}, "");
    REQUIRE(popts.HasErrorMatches());
  }

  SECTION("Options") {
    auto opt = popts.String({"-o"}, "", "");
    REQUIRE(popts.HasErrorMatches());
    REQUIRE(opt == "x"s);
  }

  SECTION("Missing argument") {
    auto opt = popts.String({"-u"}, "foobar", "");
    REQUIRE(popts.HasErrorMatches());
    REQUIRE(opt == "foobar"s);
  }
}

TEST_CASE("Holes in the tail", "[errors]") {
  popts::Options popts(vector<string>({"path/cmd", "-f", "x", "-g", "y"}));
  popts.Flag({"-f"}, "");
  popts.Flag({"-g"}, "");
  REQUIRE(!popts.HasConsistentTail());
}

TEST_CASE("Name consumed as argument", "[errors]") {
  popts::Options popts(vector<string>({"pathcmd", "-f", "-g", "x"}));
  popts.String({"-f"}, "", "");
  popts.String({"-g"}, "", "");
  REQUIRE(popts.HasErrorMatches());
}
