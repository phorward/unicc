# C target source

The contents of this folder are resulting in the file ``../c.tlt``.

The C target enables UniCC to support the C programming language in its
program module generator. Using this target, UniCC is capable to generate
parsers expressed in C and using C semantics.

The generated parsers also provides facilities for further grammar processing
and integration of the generated modules with other C modules.

The C target is also used by UniCC itself for bootstrap, meaning that UniCC
constructs its own parser out of itself.

This target is

- compliant to ANSI C89
- well tested
- based on the C standard library only
- thread-safe, parsers can recursively be called using an extendible
  Parser Control Block (pcb)
- wide-character and UTF-8 Unicode input support (C99)
- trace and stack trace facilities
- build-in error recovery
- build-in syntax tree generator
- symbol and production tables for debug and syntax tree construction
- provides a default parser test environment if no semantic code is given
- dynamic end-of-file behavior
