# Changelog

This file is used to document any relevant changes done to UniCC.

## v1.3

Released on: Not released yet.

- New command-line parameter "-l" or "--language" to specify a target language
  via command-line. This is useful when a grammar only contains AST definitions,
  which allows to render in any target.
- Abstract syntax tree generation support:
  - Drafting an abstract syntax tree definition syntax for the grammar language,
    by using the "=" operator.
  - See examples/expr.ast.par for an example working with C and Python
  - C and Python parser targets extended to construct abstract syntax tree
    data structures dynamically.
  - These are first attempts for an upcoming UniCC v2, where the targets shall
    be re-used with only few improvements.
- Fixed several bugs in the Python target on insensitive mode.
- Additionally allow ":" in case of "->" in grammar definitions.
- Internal code revisions started, but interrupted for now.
- Renamed folder "templates/" into "targets/".

## v1.2

Released on: December 7, 2017

- Started an (yet incomplete) new standard template to provide parser generation
  support for the Python programming language (targets/python.tlt).
- Imported the source code of the C standard template into the UniCC repository
  for further developments, the previous repository gets closed.
  (tempates/c.tlt)
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
