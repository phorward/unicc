# C.tlt sources

The UniCC Standard C Parser Template enables UniCC to support the C programming
language in its program module generator.

Using this template, UniCC is capable to generate parsers expressed in the
C programming language from a UniCC Grammar Definition describing a context-free
grammar for a language.

The template also provides facilities for further grammar processing and
integration of the generated modules with other C modules.

The UniCC Standard C Parser Template is also used by UniCC itself for bootstrap,
meaning that UniCC constructs its own parser out of itself.

## Features

The UniCC Standard C Parser Template provides the following features.

- Well tested, feature proved, used by UniCC's own grammar parser
- Platform and C-compiler independent, based on the C standard library only
- ANSI C89 compliant
- Thread-safe, parsers can recursively be called using an extendible
  Parser Control Block (pcb)
- Wide-character and UTF-8 Unicode input support (C99)
- Trace and stack trace facilities
- Build-in error recovery
- Build-in syntax tree generator
- Symbol and production tables for debug and syntax tree construction
- Provides a default parser test environment if no semantic code is given
- Dynamic end-of-file behavior
