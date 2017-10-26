**Cparser**: UniCC template for the C programming language.

# ABOUT #

The UniCC Standard C Parser Template enables [UniCC](https://github.com/phorward/unicc)
to support the C programming language in its program module generator.

Using this template, UniCC is capable to generate parsers expressed in the
C programming language from a UniCC Grammar Definition describing a context-free
grammar for a language.

The template also provides facilities for further grammar processing and
integration of the generated modules with other C modules.

The UniCC Standard C Parser Template is also used by UniCC itself for bootstrap,
meaning that UniCC constructs its own parser out of itself.

# FEATURES #

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

# AUTHOR #

The UniCC LALR(1) Parser Generator and all subsequent repositories and
tools is written and maintained by Jan Max Meyer, Phorward Software
Technologies.

Some other projects by the author are:

-   *pynetree* (http://pynetree.org): A light-weight parsing toolkit
    written in pure Python.
-   *phorward* (http://phorward.phorward-software.com): A free toolkit
    for parser development, lexical analysis, regular expressions and
    more.
-   *JS/CC* (http://jscc.brobston.com): The JavaScript parser generator.

# LICENSING #

Copyright (C) 2006-2017 by Phorward Software Technologies, Jan Max Meyer.

You may use, modify and distribute this software under the terms and
conditions of the 3-clause BSD license. The full license terms can be
obtained from the file LICENSE.

THIS SOFTWARE IS PROVIDED BY JAN MAX MEYER (PHORWARD SOFTWARE
TECHNOLOGIES) AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JAN
MAX MEYER (PHORWARD SOFTWARE TECHNOLOGIES) BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
