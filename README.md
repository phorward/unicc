# UniCC [![Build Status](https://travis-ci.org/phorward/unicc.svg?branch=develop)](https://travis-ci.org/phorward/unicc)

**UniCC** is a universal LALR(1) parser generator, targetting C, C++, Python, JavaScript, JSON and XML.

## About

UniCC (UNIversal Compiler Compiler) is a LALR(1) parser generator. It compiles an augmented grammar definition into a program source code that parses that grammar. Parsing is the process of transferring input matching a particular grammar, like e.g. a source code written in a programming language, into a well-formed data structure. Because UniCC is intended to be target-language independent, it can be configured via template definition files to emit parsers in nearly any programming language.

UniCC comes with out of the box support for the programming languages **C**, **C++**, **Python** and **JavaScript**. Parsers can also be generated in **JSON** and **XML**.

More targets can easily be added by creating specific template files.

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

More real-world examples for parsers implemented with UniCC are [xpl](https://github.com/phorward/xpl) and [rapidbatch](https://github.com/phorward/rapidbatch), [ViUR logics](https://github.com/viur-framework/logics) or can be found in the [examples-folder](https://github.com/phorward/unicc/tree/develop/examples).

## Features

UniCC features the following features, tools and possibilities.

- Powerful BNF-based grammar definition language
- Generates parsers in C, C++, Python, JavaScript, JSON and XML
- Build-in scanner generator with full Unicode support
- Grammar prototyping features
- Abstract Syntax Tree notation features
- Virtual productions
- Anonymous nonterminals
- Semantically determined symbols
- Platform-independent (console-based)
- Standard LALR(1) conflict resolution

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

Copyright (C) 2006-2018 by Phorward Software Technologies, Jan Max Meyer.

You may use, modify and distribute this software under the terms and conditions of the 3-clause BSD license. The full license terms can be obtained from the file LICENSE.
