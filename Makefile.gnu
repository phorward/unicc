# Standard GNU Makefile for the generic development environment at
# Phorward Software (no autotools, etc. wanted in here).

CFLAGS 			= -g -DUTF8 -DUNICODE -DDEBUG -Wall -I. $(CLOCAL)

SOURCES	= 	\
	lib/phorward.c \
	src/build.c \
	src/debug.c \
	src/error.c \
	src/first.c \
	src/integrity.c \
	src/lalr.c \
	src/lex.c \
	src/list.c \
	src/main.c \
	src/mem.c \
	src/parse.c \
	src/rewrite.c \
	src/string.c \
	src/utils.c \
	src/virtual.c \
	src/xml.c

OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

all: unicc

clean:
	-rm src/*.o
	-rm unicc

src/proto.h:
	lib/pproto src/*.c | awk "/int _parse/ { next } { print }" >$@

src/parse.c src/parse.h: src/parse.par
	unicc -o src/parse src/parse.par

make_install:
	cp Makefile.gnu Makefile

make_uninstall:
	-rm -f Makefile

# --- UniCC --------------------------------------------------------------------

unicc: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

# --- UniCC Documentation ------------------------------------------------------
#
# Now documentation generation follows, using txt2tags.
#

doc: unicc.man

unicc.man: unicc.t2t
	txt2tags -t man -o $@ $?

# --- UniCC CI Test Suite ------------------------------------------------------

TESTPREFIX=test_
TESTEXPR="42 * 23 + 1337"
TESTRESULT="= 2303"

# C

$(TESTPREFIX)c_expr:
	./unicc -o $@ examples/expr.c.par
	cc -o $@  $@.c
	test "`echo $(TESTEXPR) | ./$@ -sl`" = $(TESTRESULT)

$(TESTPREFIX)c_ast:
	./unicc -o $@ examples/expr.ast.par
	cc -o $@ $@.c
	echo $(TESTEXPR) | ./$@ -sl

test_c: $(TESTPREFIX)c_expr $(TESTPREFIX)c_ast
	@echo "--- $@ succeeded ---"
	@rm $(TESTPREFIX)*


# C++

$(TESTPREFIX)cpp_expr:
	./unicc -o $@ examples/expr.cpp.par
	g++ -o $@  $@.cpp
	test "`echo $(TESTEXPR) | ./$@ -sl`" = $(TESTRESULT)

$(TESTPREFIX)cpp_ast:
	./unicc -l C++ -o $@ examples/expr.ast.par
	g++ -o $@ $@.cpp
	echo $(TESTEXPR) | ./$@ -sl

test_cpp: $(TESTPREFIX)cpp_expr $(TESTPREFIX)cpp_ast
	@echo "--- $@ succeeded ---"
	@rm $(TESTPREFIX)*

# Python

$(TESTPREFIX)py_expr:
	./unicc -o $@ examples/expr.py.par
	-test "`python2 $@.py $(TESTEXPR) | head -n 1`" = $(TESTRESULT)
	test "`python3 $@.py $(TESTEXPR) | head -n 1`" = $(TESTRESULT)

$(TESTPREFIX)py_ast:
	./unicc -l Python -o $@ examples/expr.ast.par
	-python2 $@.py $(TESTEXPR)
	python3 $@.py $(TESTEXPR)

test_py: $(TESTPREFIX)py_expr $(TESTPREFIX)py_ast
	@echo "--- $@ succeeded ---"
	@rm $(TESTPREFIX)*

# JavaScript

$(TESTPREFIX)js_expr:
	./unicc -wt examples/expr.js.par >$@.mjs
	@echo "var p = new Parser(); p.parse(process.argv[2]);" >>$@.mjs
	test "`node $@.mjs $(TESTEXPR) | head -n 1`" = $(TESTRESULT)

$(TESTPREFIX)js_ast:
	./unicc -wtl JavaScript examples/expr.ast.par >$@.mjs
	@echo "var p = new Parser(); var t = p.parse(process.argv[2]); t.dump();" >>$@.mjs
	node $@.mjs $(TESTEXPR)

test_js: $(TESTPREFIX)js_expr $(TESTPREFIX)js_ast
	@echo "--- $@ succeded ---"
	@rm $(TESTPREFIX)*

# JSON

$(TESTPREFIX)json_ast:
	./unicc -wtl json examples/expr.ast.par >$@.json
	jq . $@.json

test_json: $(TESTPREFIX)js_expr $(TESTPREFIX)json_ast
	@echo "--- $@ succeded ---"
	@rm $(TESTPREFIX)*

# Test

test: test_c test_cpp test_py test_js test_json
	@echo "=== $+ succeeded ==="
