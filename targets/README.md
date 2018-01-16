# Target language templates

This folder holds the target language templates (tlt-files) for the different
targets supported by UniCC. A target language template is an XML-formatted text
file that contains several code snippets to be glued together into the later
running source code files.

To create a new target, just define a new tlt-file with the name of the
language.

_c.tlt_, the target for the C programming language, contains a lots of XML
comments that serve as a full documentation about which variables are used in
which contexts, and how they are made up. Please refer to this file when adding
new targets is wanted, or simply ask for help.

## C (c.tlt) / C++ (c++.tlt)

The tlt-files for C and C++ are generated from their respective source folders,
which are _C.source_ and _C++.source_. Run the Makefile there to build the
files from the different source files.

## Python (python.tlt)

The Python target stands on its own, and is not generated from a folder.

## ECMAScript (javascript.tlt)

Is currently only a stub on its own.
