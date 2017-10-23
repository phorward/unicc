# Changelog

This file is used to document any relevant changes done to UniCC.

## v1.2

Released on: Outstanding

- Fixed bug in the lexical analyzer generation by linking against
  libphorward 0.20 that caused a massive overhead of deterministic finite
  automation states in context-sensitive mode due an arbitrary character
  set order. This bug did not produce invalid parsers, but much huger ones.
- Several internal revisions
  - Changed internal names for files, functions and variables.
  - Replaced old-style function headers by more convenient ones.
  - Removed unused code.
- New README.md and updated CHANGELOG.md

## v1.1

Released on: Sep 9, 2016

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
