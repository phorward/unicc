<div align="center">
    <img src="https://github.com/phorward/unicc/raw/main/unicc.svg" height="196" alt="UniCC Logo" title="UniCC logo">
    <h1>Universal LALR(1) Parser Generator</h1>
    <a href="https://github.com/phorward/unicc/actions/workflows/test.yml">
        <img src="https://github.com/phorward/unicc/actions/workflows/test.yml/badge.svg" alt="Badge displaying the test status" title="Test badge">
    </a>
    <a href="https://github.com/phorward/unicc/LICENSE">
        <img src="https://img.shields.io/badge/License-MIT-green.svg" alt="Badge displaying the license" title="License badge">
    </a>
    <br>
    The universal LALR(1) parser generator with built-in scanner generator,<br>
    creating parsers in different target programming languages.
</div>

## About

**unicc** is a parser generator that compiles an extended grammar definition into program source code that parses the described grammar. Since UniCC is target language independent, it can be configured via template definition files to generate parsers in any programming language.

UniCC natively supports the programming languages **C**, **C++**, **Python** and **JavaScript**. Parse tables can also be generated to **JSON**. Parsers for other programming languages can be easily adapted.

UniCC is capable to generate both scannerless parsers and parsers with a separate scanner. The more powerful scannerless parsing is the default and allows the barrier between the grammar and its tokens to be broken, leaving the tokens under the full control of the context-free grammar. Scannerless parsing requires that the provided grammar is rewritten internally according to the whitespace and lexeme settings.

## Examples

Below is the full definition of a simple, universal grammar example that can be compiled to any of UniCC's target languages.

This example uses the automatic abstract syntax tree construction syntax to define nodes and leafs of the resulting syntax tree.

```unicc
%whitespaces    ' \t';

%left           '+' '-';
%left           '*' '/';

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

Next is a (more complex) version of the four-function arithmetic syntax including their calculation semantics, for integer values. In this example, the scannerless parsing capabilities of UniCC are used to parse the **int** value from its single characters, so the symbol **int** is configured to be handled as a `lexeme`, which influences the behavior of how whitespace is handled.

```unicc
%!language      C;	// <- target language!

%whitespaces    ' \t';
%lexeme         int;
%default action [* @@ = @1 *];

%left           '+' '-';
%left           '*' '/';

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

To build this example, run the following commands

```bash
$ unicc expr.par
$ cc -o expr expr.c
```

Afterwards, you can run the expression parser like this
```bash
$ ./expr -sl
42 * 23 + 1337
= 2303
```

This [C](examples/expr.c.par)-example can also be found for [C++](examples/expr.cpp.par), [Python](examples/expr.py.par) and [JavaScript](examples/expr.js.par).

More real-world examples for parsers implemented with UniCC can be found in [XPL](https://github.com/phorward/xpl), [RapidBATCH](https://github.com/phorward/rapidbatch) and [Logics](https://github.com/viur-framework/logics).

## Features

UniCC provides the following features and tools:

- Grammars are expressed in a powerful Backus-Naur-style meta language
- Generates standalone (dependency-less) parsers in
  - C
  - C++
  - Python (>= 2.7, tested until 3.11)
  - JavaScript (ES2018)
- Provides facilities to generate parse tables into JSON
- Scannerless parser supported by default
- Full Unicode processing built-in
- Grammar prototyping features
  - automatic grammar revision for scannerless parsers
  - virtual productions
  - anonymous nonterminals
- Abstract syntax tree notation features
- Semantically determined symbols
- Standard LALR(1) conflict resolution

## Documentation

The [UniCC User's Manual](http://downloads.phorward-software.com/unicc/unicc.pdf) is the official standard documentation of the UniCC Parser Generator.

## Installation

UniCC can be build and installed like any GNU-style program, with

```bash
$ . /configure
$ make
$ make install
```

Alternatively, the dev-toolchain can be used, by just calling on any recent Linux system.

```bash
$ touch src/parse.?  # you have to do this only once
$ make -f Makefile.gnu
```

## License

UniCC is free software under the MIT license.<br>
Please see the LICENSE file for more details.
