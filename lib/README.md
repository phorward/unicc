# phorward v1.0.3

This is a stand-alone copy of the Phorward C/C++ library that was generated on 2021-01-20 from the [official phorward repository](https://github.com/phorward/libphorward).

It is not intended to be used for further development, but for simple and fast integration into existing projects. To contribute changes, please check out the official repository at https://github.com/phorward/libphorward.

Thank you & have fun!

## Data structures

- [parray](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#parray) - Dynamically managed arrays & stacks
- [pccl](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#pccl) - Character-classes
- [plex](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#plex) - Lexical analysis
- [plist](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#plist) - Linked lists, hash-tables, queues & stacks
- [pregex](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#pregex) - Regular expressions

## Generic helpers

- [DEBUG-facilities](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#ptrace) - Logging, tracing and run-time analysis
- [pgetopt](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#fn_pgetopt) - Command-line options interpreter
- [pstr*, pwcs*](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#pstr) - Enhanced string operations

## Command-line tools

- [pdoc](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#c_pdoc) - C source code documentation tool
- [pinclude](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#c_pinclude) - Generate big files from various smaller ones
- [plex](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#c_plex) - Lexical analyzer generator and interpreter
- [pproto](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#c_pproto) - C function prototype generator
- [pregex](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#c_pregex) - Regular expressions match/find/split/replace
- [ptest](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html#c_ptest) - C program test facilities

## Documentation

A recently updated, [full documentation can be found here](https://raw.githack.com/phorward/libphorward/master/doc/phorward.html), and is also locally available after installation.

## Building

Building *phorward* is simple as every GNU-style open source program. Extract the downloaded release tarball or clone the source repository into a directory of your choice.

Then, run

```bash
./configure
make
make install
```

And you're ready to go!

### Alternative development build

Alternatively, there is also a simpler method for setting up a local build system for development and testing purposes.

To do so, type

```bash
make -f Makefile.gnu make_install
make
```

This locally compiles the library or parts of it, and is ideal for development purposes.

### Stand-alone copy

The entire library including its tools can be made available in one target directory by using the script `./standalone.sh`.

This makes stand-alone integration of the entire library into other projects possible without a previous installation or porting, and easier maintainable packages.

The generated stand-alone package contains a `Makefile` and can directly be built.

## Credits

*libphorward* is developed and maintained by [Jan Max Meyer](https://github.com/phorward/), Phorward Software Technologies.

Contributions by [Heavenfighter](https://github.com/Heavenfighter) and [AGS](https://github.com/FreeBASIC-programmer).

## License

You may use, modify and distribute this software under the terms and conditions of the MIT license.
The full license terms can be obtained from the file LICENSE.

Copyright (C) 2006-2021 by Phorward Software Technologies, Jan Max Meyer.
