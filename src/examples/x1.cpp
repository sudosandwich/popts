#include "popts.hpp"

#include <fstream>
#include <iostream>

using namespace std;

bool Process(istream &in, ostream &out) {
  char c;
  while (in >> c) {
  }
  return true;
}

int main(int argc, char **argv) {
  popts::Options popts(argc, argv);

  auto help = popts.Flag({"-h", "--help"}, "Show this help");

  auto infile = popts.String({"-i", "--infile"}, "--",
                             "Specify input file or '--' for stdin");

  auto outfile = popts.String({"-o", "--outfile"}, "--",
                              "Specify output file or '--' for stdout");

  auto verbose = popts.Flag({"-v"}, "Toggle verbosity");

  if (help) {
    cout << popts.Description() << "\n";
    return 0;
  }

  istream *in = &cin;
  ostream *out = &cout;
  ifstream filein;
  ofstream fileout;

  if (infile != "--") {
    filein.open(infile);
    in = &filein;
  }

  if (outfile != "--") {
    fileout.open(outfile);
    out = &fileout;
  }

  if (!Process(*in, *out)) {
    if (verbose) {
      cerr << "An error occurred.";
    }
    return 1;
  }

  return 0;
}
