# Changelog

This file is used to document any relevant changes done to UniCC.

## v1.5

Yet unreleased.

- New targets for JavaScript and JSON.
- Targets C and C++ improved to avoid memory leaking and handle malloc/realloc
  calls more securely.
- All targets now supporting the `UNICC_SUCCESS` and `UNICC_ERROR` flags that
  can be set as current `act` (action) to stop parsing.
- Started a grammar and templates test suite for better continous integration.
- Compiled and linked against libphorward 0.23.
- Removed outdated syntax-tree feature from C target.

## v1.4

Released on: April 17, 2018

- Compiled and linked against libphorward 0.22
- C++ target forked from the C target startet
- *action_prefix* value can be defined in target language templates to
  redefine the ``@``-prefix for action code variables by any other desired
  prefix. This change has been done to make target language processing easier
  for languages where the ``@``-sign is part of the target language syntax.

## v1.3

Released on: December 21, 2017

- v1.3.2: Python target 0.4
  - Raising parse error exceptions
  - Bugfix to avoid working with previous stack values
- v1.3.1: Bugfix in the C target working with uninitialized values
- Code generator does not emit action code when the action is empty.
- New command-line parameter "-l" or "--language" to specify a target language
  via command-line. This is useful when a grammar only contains AST definitions,
  which allows to render and run it in any target.
- Abstract syntax tree generation support:
  - Drafting an abstract syntax tree definition syntax for the grammar language,
    by using the operators "=" and ":=". See examples/expr.ast.par for an
    example working under both C and Python without any modifications.
  - C and Python parser targets extended to construct abstract syntax tree
    data structures dynamically.
  - These are first changes for an upcoming UniCC v2, where the targets shall
    be re-used with only few improvements.
- Fixed several bugs in the Python target on insensitive mode.
- Additionally allow ":" in case of "->" in grammar definitions.
- Internal code revisions started, but interrupted for now.
- Renamed folder "templates/" into "targets/".

## v1.2

Released on: November 7, 2017

- Started an (yet incomplete) new standard template to provide parser generation
  support for the Python programming language (targets/python.tlt).
- Imported the source code of the C standard template into the UniCC repository
  for further developments, the previous repository gets closed.
  (targets/c.tlt)
- Fixed bug in the lexical analyzer generation by linking against
  libphorward 0.20 that caused a massive overhead of deterministic finite
  automation states in context-sensitive mode due an arbitrary character
  set order. This bug did not produce invalid parsers, but horribly huge tables.
- Imported the source code of min_lalr1 into the unicc repository, because it
  is the only place where it is used.
- Several internal revisions
  - Changed internal names for files, functions and variables.
  - Replaced old-style function headers by more convenient ones.
  - Removed unused code.
- Renewed manpage
- New README.md and updated CHANGELOG.md

## v1.1

Released on: September 9, 2016

- Internal fixes to compiles against libphorward 0.18
- Licensing terms changed from Artistic License 2.0 to 3-clause BSD-license

## v1.0

Released on: June 29, 2012

- Moved build toolchain to GNU autotools, cross-compiles well on Linux
- Turned the entire regular expression handling to the new pregex_ptn structures
  of the Phorward Foundation Library.
- Output of original regular expressions in the standard regex notation into
  the <regex>-tag of the parser description file.
- New command-line option '-t' to print the output files to stdout instead of
  writing it into files.
- Command-line option '-v' automatically switches '-s' on.
- Added document type definition file 'unicc.dtd' and changed XML output.
- The "#case insensitive strings" option was not correctly recognized due the
  revision of the regular pattern construction mechanism.
