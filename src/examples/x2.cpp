#include "popts.hpp"

#include <complex>
#include <iostream>

int main(int argc, char **argv) {
  using compl = std::complex<double>;

  popts::Options popts(argc, argv);
  auto c = popts.MakeOption<compl>({"-c"}, compl(0, 0), "A complex number");
  std::cout << c;
  return 0;
}
