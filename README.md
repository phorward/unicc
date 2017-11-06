# UniCC [![Build Status](https://travis-ci.org/phorward/unicc.svg?branch=develop)](https://travis-ci.org/phorward/unicc)

**UniCC** is a target-language independent parser generator.

## About

UniCC is a parser generator. It compiles an augmented grammar definition into a program source code. Because UniCC is intended to be a target-language independent parser generator, it can be configured via template definition files to emit parsers in probably any programming language.

UniCC supports parser code generation for the following programming languages so far:

- **C** is fully supported (and reference implementation),
- **Python** is under development, but already running in first tests,
- **ECMAScript** is prototyped in a stub, but may come soon.

More target languages can easily be added by creating specific target language templates.

## Example

This is the full definition for a four-function arithmetic syntax including their integer calculation semantics (in C).

```c
#!language      "C";	// <- target language!

#whitespaces    ' \t';
#lexeme         int;
#default action [* @@ = @1; *];

#left           '+' '-';
#left           '*' '/';

//Defining the grammar
calc$           -> expr          [* printf( "= %d\n", @expr ); *]
                ;

expr            -> expr '+' expr [* @@ = @1 + @3; *]
                | expr '-' expr  [* @@ = @1 - @3; *]
                | expr '*' expr  [* @@ = @1 * @3; *]
                | expr '/' expr  [* @@ = @1 / @3; *]
                | '(' expr ')'   [* @@ = @2; *]
                | int
                ;

int             -> '0-9'         [* @@ = @1 - '0'; *]
                | int '0-9'      [* @@ = @int * 10 + @2 - '0'; *]
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

More real-world examples for programming languages implemented using UniCC are [xpl](https://github.com/phorward/xpl) and [rapidbatch](https://github.com/phorward/rapidbatch).

## Features

UniCC features the following, unique tools and possibilities.

- Powerful BNF-based grammar definition language
- Full unicode support
- Build-in lexical analyzer
- Grammar prototyping features
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
- Supporting C and Python target languages so far

## Documentation

The [UniCC User's Manual](https://www.phorward-software.com/products/unicc-lalr1-parser-generator/user-manual_index.html) is the official documentation of the UniCC Parser Generator. Download it for free [here](https://www.phorward-software.com/products/unicc/unicc.pdf).

## Installation

UniCC can be build and installed like any GNU-style program, with

```sh
./configure
make
make install
```

Previously, the [Phorward Toolkit](https://github.com/phorward/phorward) must be compiled and installed, because UniCC depends on it.

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
