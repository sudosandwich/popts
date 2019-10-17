# popts - A Simple C++ Program Options Library

## Rationale
There are many libraries for parsing program options out there.
The ones I know I quite unwieldy offering many options.
I often write small command line programs that require some options. 
Usually the use-case is too complex to just look at argv but too simple to add a dependency to, for example `Boost.ProgramOptions`.

`popts` attempts to close this gap.
Some design goals:

* no exceptions, errors can be queried
* no virtual functions
* single header (automatically generated)
* requires a modern compiler, e.g. ranged for loops and `<chrono>` should be available
* consistent program state even after errors
* simple to use
* consistent use of vocabluary


## Example

Consider a program that

1. reads `stdin`, 
2. outputs the processed contents to `stdout`.

```c++
#include <iostream>

using namespace std;

bool Process(istream& in, ostream& out);

int main() {
    if(!Process(cin, out)) {
        cerr << "An error occurred.";
        return 1;
    }
    return 0;
}
```

Let's modify this program so it takes

* an optional infile,
* an optional outfile,
* a flag to signal verbose output,
* and, hey why not, a help option.

```c++
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
```

A help text can be generated with `cout << popts.Description() << "\n";`.

Output:
```
╰─$ ./x1 -h
Usage 'x1' [options]
-h, --help             Show this help
-i, --infile [=--]     Specify input file or '--' for stdin
-o, --outfile [=--]    Specify output file or '--' for stdout
-v                     Toggle verbosity
```

## Reference

### Nomenclature

The structure of a command is 

```sh
./cmd --flag --name argument tail1 tail2
```

* An *option* is either `--flag` or `--name argument`.
* An option has one or multiple *name*s and one *argument*.
* A *flag* is an option without an argument.
* The *tail* is the rest of `argv` starting at the highest index that has been parsed.

> N.B. This means that if, during `cmd --not-a-flag --flag tail`, the option `--not-a-flag` has not been parsed but `--flag` has, then tail will only contain `tail`. 
> You can use `popts.HasConsistentTail()` to check for this.
> See the errors section.


### Single and Multiple Arguments

For every 

```c++
const type& Options::Type(names, default, description);
``` 

there is a corresponding 

```c++
const std::deque<type>& Options::Types(names, description);
``` 

function.
The references returned are bound to the lifetime of the `Options` object and are not invalidated by function calls.
The container version does not have a default and can hence return an empty container.

Consider `./cmd -f x -f y`.

If `-f` has been declared using 

```c++
auto opt = String({"-f"}, "", "");
```

then `HasErrorMatches()` will return `true` and the first occurence will be returned and `opt == "x"s`.

If `-f` has been declared using 

```c++
auto opt = Strings({"-f"}, "");
```

 then `HasErrorMatches()` will return `false` and all occurences will be returned and `opt == deque<string>{"x"s, "y"s}`.

 The same logic goes for `Flag(...)` and `Flags(...)`.


### Custom Types

You can use custom types using the `MakeOption` and `MakeOptions` interfaces. 
The requirements for `Type` are 
* default constructible
* copy-assignable
* streamable (i.e. implements `ostream& operator<<(ostream&, const Type&)` and `istream& operator>>(istream&, const Type&)`).

For example the `complex` class from the standard supports this.

```c++
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
```

Now we can use the parsing capabilities of the standard library.

```sh
╰─$ ./x2
(0,0)
╰─$ ./x2 -c "(1,2)"
(1,2)
```

For custom types, implementing the streaming operators should suffice to be able to use them in `popts`.


### Tail

It is common to treat trailing arguments as positional arguments.
To access the remainder of the unparsed `argv`, use `Tail`.

```c++
if(popts.HasConsistentTail(&cerr))
{
    handle_remaining_args(popts.Tail().cbegin(), popts.Tail().cend());
}
```


### Errors

Since `popts` does not use exceptions, it is your duty to check for errors.
The error checking functions take an optional `ostream*` argument, where errors are reported.


If a name occurs multiple times in flag definitions, it is a *programmer error*.
Check for this using `HasDuplicateNames`.

```c++
auto f1 = popts.Flag({"-f"}, "", "");
auto f2 = popts.Flag({"-f"}, "", "");
assert(popts.HasDuplicateNames(&cerr) == true);
```

During debug builds, this is checked via `assert`.

A *user error* can be detected using `HasErrorMatches`.
Consider `./cmd -i -g` where both options are strings.

`HasErrorMatches` checks the following errors:
- The argument of `-i` is `-g`.
- The argument of `-g` is `nullptr`.
- A single option or flag occurs multiple times.
- A value could not be parsed from `string` to `Type`.

Finally to check if everything until `Tail().cbegin()` has been processed, call `HasConsistentTail(&cerr)`.
It will report unparsed arguments.


### Note on Duration

Since as the time of writing this, `<chrono>` does not have streaming operators yet, the library povides `Duration` functions.
They return a floating point, second based `std::duration` suitable for (imprecisely) storing a wide range of durations.
It can be converted freely to other duration types.

```c++
void sleep(chrono::seconds s);

int main(int argc, char** argv)
{
    popts::Options popts(argc, argv);
    auto howLong = popts.Duration(
        {"-d"}, 
        chrono::milliseconds(100), 
        "Sleep time");
    sleep(howLong);
    return 0;
}
```

```sh
╰─$ ./cmd -d 5ms
╰─$ ./cmd -d 4h
```


