# UniCC [![Build Status](https://travis-ci.org/phorward/unicc.svg?branch=develop)](https://travis-ci.org/phorward/unicc)

**UniCC** is a target-language independent parser generator.

## About

UniCC is a parser generator. It compiles an augmented grammar definition into a program source code. Because UniCC is intended to be a target-language independent parser generator, it can be configured via template definition files to emit parser programs in probably any programming language.

Currently, UniCC does support the C programming language using its [Standard C Parser Template](https://github.com/phorward/Cparser) only.

## Example

This is the full definition for a four-function arithmetic syntax including their integer calculation semantics.

```c
//Some grammar-related directives
#!language      "C";

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
- Supporting the C programming language via the UniCC Standard C Parser
Template, providing many useful features like integrated syntax tree
visualizer and error recovery

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
- [pynetree](https://github.com/phorward/pynetree), a light-weight parsing toolkit written in pure Python.
- [RapidBATCH](https://github.com/phorward/rapidbatch), a scripting language.
- [JS/CC](https://jscc.brobston.com), the JavaScript parser generator.

## License

This software is an open source project released under the terms and conditions of the 3-clause BSD license. See the LICENSE file for more information.

Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer.

You may use, modify and distribute this software under the terms and conditions of the 3-clause BSD license. The full license terms can be obtained from the file LICENSE.
