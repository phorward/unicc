# UniCC [![Build Status](https://travis-ci.org/phorward/unicc.svg?branch=master)](https://travis-ci.org/phorward/unicc)

**UniCC** is a target-language independent parser generator.

## About

UniCC (UNIversal Compiler Compiler) is a LALR(1) parser generator. It compiles an augmented grammar definition into a program source code that parses that grammar. Parsing is the process of transferring input matching a particular grammar, like e.g. a source code written in a programming language, into a well-formed data structure. Because UniCC is intended to be target-language independent, it can be configured via template definition files to emit parsers in nearly any programming language.

UniCC supports parser code generation for the following programming languages so far:

- **C** is fully supported (and reference implementation),
- **Python** is well supported,
- **C++** and **ECMAScript** are prototyped, and will come soon.

More target languages can easily be added by creating specific target language templates.

## Example

This is the full definition for a four-function arithmetic syntax including their integer calculation semantics (in C).

```c
#!language      "C";	// <- target language!

#whitespaces    ' \t';
#lexeme         int;
#default action [* @@ = @1 *];

#left           '+' '-';
#left           '*' '/';

//Defining the grammar
calc$           : expr           [* printf( "= %d\n", @expr ) *]
                ;

expr            : expr '+' expr  [* @@ = @1 + @3 *]
                | expr '-' expr  [* @@ = @1 - @3 *]
                | expr '*' expr  [* @@ = @1 * @3 *]
                | expr '/' expr  [* @@ = @1 / @3 *]
                | '(' expr ')'   [* @@ = @2 *]
                | int
                ;

int             : '0-9'          [* @@ = @1 - '0' *]
                | int '0-9'      [* @@ = @int * 10 + @2 - '0' *]
                ;
```

To build and run this example, do

```
$ unicc expr.par
$ cc -o expr expr.c
$ ./expr -sl
3*10-(2*4)+1
= 23
```

More real-world examples for parsers implemented with UniCC can are [xpl](https://github.com/phorward/xpl) and [rapidbatch](https://github.com/phorward/rapidbatch), or can be found in the [examples-folder](https://github.com/phorward/unicc/tree/develop/examples).

## Features

UniCC features the following features, tools and possibilities.

- Powerful BNF-based grammar definition language
- Full unicode support
- Build-in lexical analyzer
- Grammar prototyping features
- Abstract Syntax Tree notations
- Virtual productions
- Anonymous nonterminals
- Semantically determined symbols
- Two parser construction modes allow the use of different algorithmic
approaches relating the whitespace handling
- Target-language independent parser development
- Template-based program-module generator and XML-based parser description
file generator
- Platform-independent (console-based)
- Standard LALR(1) conflict resolution
- Parser generation in C, C++ and Python so far

## Documentation

The [UniCC User's Manual](https://www.phorward-software.com/products/unicc-lalr1-parser-generator/user-manual_index.html) is the official documentation of the UniCC Parser Generator. Download it for free [here](https://www.phorward-software.com/products/unicc/unicc.pdf).

## Installation

On Linux and OS X, UniCC can be build and installed like any GNU-style program, with

```sh
./configure
make
make install
```

Previously, the [Phorward Toolkit](https://github.com/phorward/phorward) must be compiled and installed, because UniCC depends on it.

Windows users may checkout the pre-built setup package that can be found on the Phorward download server at https://phorward.info/download/unicc.

## Contributions

Contributions, ideas, concepts and code is always welcome!
Please feel free to contact us if you have any questions.

## Credits

UniCC is developed and maintained by Jan Max Meyer, Phorward Software Technologies.

Some other projects by the author are:

- [libphorward](https://github.com/phorward/phorward), a free toolkit for parser development, lexical analysis, regular expressions and more.
- [RapidBATCH](https://github.com/phorward/rapidbatch), a scripting language.
- [pynetree](https://github.com/phorward/pynetree), a light-weight parsing toolkit written in pure Python.
- [JS/CC](http://jscc.brobston.com), the JavaScript parser generator.

## License

This software is an open source project released under the terms and conditions of the 3-clause BSD license. See the LICENSE file for more information.

Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer.

You may use, modify and distribute this software under the terms and conditions of the 3-clause BSD license. The full license terms can be obtained from the file LICENSE.
