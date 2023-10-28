#
# Automake targets
#

bin_PROGRAMS = unicc$(EXEEXT)

man1_MANS = unicc.man

uniccdir = $(datadir)/unicc
targetsdir = $(uniccdir)/targets
dist_targets_DATA = targets/c.tlt \
					targets/c++.tlt \
					targets/python.tlt \
					targets/javascript.tlt \
					targets/json.tlt

dist_unicc_DATA = LICENSE README.md

AM_CPPFLAGS = -DTLTDIR=\"$(targetsdir)\"

unicc_SOURCES = \
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
