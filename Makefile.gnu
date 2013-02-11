# Standard GNU Makefile for the generic development environment at
# Phorward Software (no autotools, etc. wanted in here).

CFLAGS 			= -I../phorward/src -DUTF8 -DUNICODE -DDEBUG -Wall $(CLOCAL)
LIBPHORWARD		= ../phorward/src/libphorward.a

SOURCES			= 	\
				p_mem.c \
				p_error.c \
				p_first.c \
				p_lalr_gen.c \
				p_util.c \
				p_string.c \
				p_integrity.c \
				p_virtual.c \
				p_rewrite.c \
				p_debug.c \
				p_keywords.c \
				p_build.c \
				p_xml.c \
				p_main.c

all: unicc

clean:
	-rm *.o
	-rm p_parse_boot1.c p_parse_boot2.c p_parse_boot2.h p_parse_boot3.c p_parse_boot3.h
	-rm unicc unicc_boot1 unicc_boot2 unicc_boot3

make:
	cp Makefile.gnu Makefile

unmake:
	-rm -f Makefile

# --- UniCC Bootstrap phase 1 --------------------------------------------------
#
# This phase uses the experimental min_lalr1 Parser Generator to build a
# rudimentary parser for UniCC. min_lalr1 must be available and compiled
# with its delivered Makefile.gnu in a directory ../min_lalr from here.
#

unicc_boot1_SOURCES = p_parse_boot1.c $(SOURCES)
unicc_boot1_OBJECTS = $(patsubst %.c,%.o,$(unicc_boot1_SOURCES))

p_parse_boot1.c: p_parse.syn
	../min_lalr1/min_lalr1 $? >$@ 2>/dev/null

unicc_boot1: $(unicc_boot1_OBJECTS) $(LIBPHORWARD)
	$(CC) -o $@ $(unicc_boot1_OBJECTS) $(LIBPHORWARD)

# --- UniCC Bootstrap phase 2 --------------------------------------------------
#
# In this phase, the parser generated by min_lalr1 is will be used to parse the
# grammar definition of the UniCC parser (p_parse.par)
#

unicc_boot2_SOURCES = p_parse_boot2.c $(SOURCES)
unicc_boot2_OBJECTS = $(patsubst %.c,%.o,$(unicc_boot2_SOURCES))

p_parse_boot2.c p_parse_boot2.h: p_parse.par unicc_boot1
	./unicc_boot1 -svwb p_parse_boot2 p_parse.par

unicc_boot2: $(unicc_boot2_OBJECTS) $(LIBPHORWARD)
	$(CC) -o $@ $(unicc_boot2_OBJECTS) $(LIBPHORWARD)

# --- UniCC Bootstrap phase 3 --------------------------------------------------
#
# In this phase, the UniCC parser compiled by UniCC will be used to build
# itself.
#

unicc_boot3_SOURCES = p_parse_boot3.c $(SOURCES)
unicc_boot3_OBJECTS = $(patsubst %.c,%.o,$(unicc_boot3_SOURCES))

p_parse_boot3.c p_parse_boot3.h: p_parse.par unicc_boot2
	./unicc_boot2 -svwb p_parse_boot3 p_parse.par

unicc_boot3: $(unicc_boot3_OBJECTS) $(LIBPHORWARD)
	$(CC) -o $@ $(unicc_boot3_OBJECTS) $(LIBPHORWARD)

# --- UniCC Final Build --------------------------------------------------------
#
# Using the third bootstrap phase, the final UniCC executable is built.
#

unicc_SOURCES = p_parse.c $(SOURCES)
unicc_OBJECTS = $(patsubst %.c,%.o,$(unicc_SOURCES))

p_parse.c p_parse.h: p_parse.par unicc_boot3
	./unicc_boot3 -svwb p_parse p_parse.par

unicc: $(unicc_OBJECTS) $(LIBPHORWARD)
	$(CC) -o $@ $(unicc_OBJECTS) $(LIBPHORWARD)

# --- UniCC Documentation ------------------------------------------------------
#
# Now documentation generation follows, using txt2tags.
#

doc: manpage README

manpage: unicc.man

README: unicc.t2t
	-rm -f $@
	txt2tags -t txt -H -o - $? | sed -E -n '1h;1!H;$${;g;s/ +([-A-Z ]+)\n +(=+)/\2==\n \1 \n\2==/g;p;}' | sed -e "/^=/s/=/*/g;1,15d" >$@.tmp
	cat unicc.hdr $@.tmp >>$@
	rm -f $@.tmp


unicc.man: unicc.t2t
	txt2tags -t man $?

