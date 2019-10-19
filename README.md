# UniCC [![Build Status](https://travis-ci.org/phorward/unicc.svg?branch=master)](https://travis-ci.org/phorward/unicc)

**UniCC** is a universal LALR(1) parser generator, targetting C, C++, Python, JavaScript, JSON and XML.

## About

UniCC (UNIversal Compiler-Compiler) is a LALR(1) parser generator.

It compiles an augmented grammar definition into a program source code that parses the described grammar. Because UniCC is intended to be target-language independent, it can be configured via template definition files to emit parsers in almost any programming language.

UniCC comes with out of the box support for the programming languages **C**, **C++**, **Python** (both 2.x and 3.x) and **JavaScript**. Parsers can also be generated into **JSON** and **XML**.

UniCC can generate both scanner-less and scanner-mode parsers. The more powerful scanner-less parsing is the default, and allows to break the barrier between the grammar and its tokens, so tokens are under full control of the context-free grammar. Scanner-less parsing requires that the provided grammar is internally rewritten according to whitespace and lexeme settings.

## Examples

Below is the full definition of a simple, universal grammar example that can be compiled to any of UniCC's target languages.
This example uses the automatic abstract syntax tree construction syntax to define nodes and leafs of the resulting syntax tree.

```c
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

```c
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

The **UniCC User's Manual** is the official standard documentation of the UniCC Parser Generator.
Download it for free [here](https://www.phorward-software.com/products/unicc/unicc.pdf).

## Installation

On Linux and OS X, UniCC can be build and installed like any GNU-style program, with

```sh
./configure
make
make install
```

Windows users may download the pre-built setup package that can be found on the Phorward download server at https://phorward.info/download/unicc.

## Contributions

Contributions, ideas, concepts and code is always welcome. Please feel free to contact me if you have any questions.

## Credits

UniCC is developed and maintained by Jan Max Meyer, Phorward Software Technologies.

## License

This software is an open source project released under the terms and conditions of the MIT license. See the LICENSE file for more information.

Copyright (C) 2006-2019 by Phorward Software Technologies, Jan Max Meyer.

You may use, modify and distribute this software under the terms and conditions of the 3-clause BSD license. The full license terms can be obtained from the file LICENSE.
