# Targets

This folder holds the target language templates ("tlt-files") for the different
targets supported by UniCC. A target language template is an XML-formatted text
file that contains several code snippets to be glued together into the later
running source code files.

To create a new target, just define a new tlt-file with the name of the
language.

_c.tlt_, the target for the C programming language, contains a lots of XML
comments that serve as a full documentation about which variables are used in
which contexts, and how they are made up. Please refer to this file when adding
new targets is wanted, or simply ask for help.

## C (c.tlt)

The C target is generated from the source folder _C.source_ .
Run the Makefile there to build the target from the different raw source files.

## C++ (c++.tlt)

The C++ target is a fork of the C target and is generated from the source folder
_C++.source_ . Run the Makefile there to build the target from the different
raw source files.

## Python (python.tlt)

The Python target stands on its own, and is not generated from a folder.

## JavaScript (javascript.tlt)

The JavaScript target stands on its own, and is not generated from a folder.
