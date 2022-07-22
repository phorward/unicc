# UniCC [![C/C++ CI](https://github.com/phorward/unicc/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/phorward/unicc/actions/workflows/c-cpp.yml) [![MIT License badge](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

**unicc** is a universal LALR(1) parser generator with built-in scanner generator, targetting C, C++, Python, JavaScript, JSON and XML.

## About

**unicc** compiles an augmented grammar definition into a program source code that parses the described grammar. Because UniCC is intended to be target-language independent, it can be configured via template definition files to emit parsers in almost any programming language.

UniCC comes with out of the box support for the programming languages **C**, **C++**, **Python** and **JavaScript**. Parsers can also be generated into **JSON** and **XML**.

UniCC can generate both scanner-less and scanner-mode parsers. The more powerful scanner-less parsing is the default, and allows to break the barrier between the grammar and its tokens, so tokens are under full control of the context-free grammar. Scanner-less parsing requires that the provided grammar is internally rewritten according to whitespace and lexeme settings.

## Examples

Below is the full definition of a simple, universal grammar example that can be compiled to any of UniCC's target languages.
This example uses the automatic abstract syntax tree construction syntax to define nodes and leafs of the resulting syntax tree.

```unicc
#whitespaces    ' \t';

#left           '+' '-';
#left           '*' '/';

@int            '0-9'+           = int;

expr$           : expr '+' expr  = add
                | expr '-' expr  = sub
                | expr '*' expr  = mul
                | expr '/' expr  = div
                | '(' expr ')'
                | @int
                ;
```

On the input `42 * 23 + 1337`, this will result in the AST output

```
add
 mul
  int (42)
  int (23)
 int (1337)
```

Next is a (more complex) version of the four-function arithmetic syntax including their calculation semantics, for integer values. In this example, the scannerless parsing capabilities of UniCC are used to parse the **int** value from its single characters, so the symbol **int** is configured to be handled as a `lexeme`, which influences the behavior how whitespace is handled.

```unicc
#!language      C;	// <- target language!

#whitespaces    ' \t';
#lexeme         int;
#default action [* @@ = @1 *];

#left           '+' '-';
#left           '*' '/';

calc$           : expr                 [* printf( "= %d\n", @expr ) *]
                ;

expr            : expr:a '+' expr:b    [* @@ = @a + @b *]
                | expr:a '-' expr:b    [* @@ = @a - @b *]
                | expr:a '*' expr:b    [* @@ = @a * @b *]
                | expr:a '/' expr:b    [* @@ = @a / @b *]
                | '(' expr ')'         [* @@ = @expr *]
                | int
                ;

int             : '0-9'                [* @@ = @1 - '0' *]
                | int '0-9'            [* @@ = @int * 10 + @2 - '0' *]
                ;
```

To build and run this example, run the following commands

```
$ unicc expr.par
$ cc -o expr expr.c
./expr -sl
42 * 23 + 1337
= 2303
```

This [C](examples/expr.c.par)-example can also be found for [C++](examples/expr.cpp.par), [Python](examples/expr.py.par) and [JavaScript](examples/expr.js.par).

More real-world examples for parsers implemented with UniCC can be found in [xpl](https://github.com/phorward/xpl), [rapidbatch](https://github.com/phorward/rapidbatch) and [ViUR logics](https://github.com/viur-framework/logics).

## Features

UniCC provides the following features and tools:

- Grammars are expressed in a powerful Backus-Naur-style meta language
- Generates parsers in C, C++, Python, JavaScript, JSON and XML
- Scanner-less and scanner-mode parser construction supported
- Build-in full Unicode processing
- Grammar prototyping features, virtual productions and anonymous nonterminals
- Abstract syntax tree notation features
- Semantically determined symbols
- Standard LALR(1) conflict resolution
- Platform-independent (console-based)

## Documentation

The [UniCC User's Manual](http://downloads.phorward-software.com/unicc/unicc.pdf) is the official standard documentation of the UniCC Parser Generator.

## Installation

On Linux and OS X, UniCC can be build and installed like any GNU-style program, with

```sh
./configure
make
make install
```

In the past, setup packages for Windows systems where also provided, but these are not maintained anymore since unicc v1.6. You can still find them [here](https://downloads.phorward-software.com/unicc/).

## UniCC v2

Between 2014 and 2020, a version 2 of UniCC was under development, but abandoned for now. This version currently exists in the [branch unicc2](https://github.com/phorward/unicc/tree/unicc2) inside of this repository, and is a complete rewrite, but with the intention to provide better tools for grammar prototyping and direct AST traversal.

## License

UniCC is free software under the MIT license.<br>
Please see the LICENSE file for more details.
