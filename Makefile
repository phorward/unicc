#-------------------------------------------------------------------------------
# UniCC LALR(1) Parser Generator
# Copyright (C) 2006-2012 by Phorward Software Technologies, Jan Max Meyer
# http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com
#
# Make-File:	Makefile
# Author:		Jan Max Meyer
#
# You may use, modify and distribute this software under the terms and condi-
# tions of the Artistic License, version 2. Please see LICENSE for more infor-
# mation.
#-------------------------------------------------------------------------------

UNICC		=	unicc

OBJEXT		=	.o
EXEEXT		=

UNICC_TPLDIR ?=	../Cparser
UNICC_ENV	=	UNICC_TPLDIR="$(UNICC_TPLDIR)"

CFLAGS		+=	-funsigned-char -DUNICODE -DUTF8

PROGRAM		=	$(UNICC)$(EXEEXT)
PROG_BOOT1	=	boot_unicc1$(EXEEXT)
PROG_BOOT2	=	boot_unicc2$(EXEEXT)
PROG_BOOT3	=	boot_unicc3$(EXEEXT)

PARSER_OUT	=	p_parse
PARSER		=	$(PARSER_OUT).c
PARSER_H	=	$(PARSER_OUT).h
PARSER_OBJ	=	$(PARSER_OUT)$(OBJEXT)

PARSER_BOOT	=	p_parse.syn
PARSER_SRC	=	p_parse.par
PARSER_DBG	=	p_parse.dbg

PROTO		=	

SRC			=	p_mem.c \
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
				
OBJ			=	p_mem$(OBJEXT) \
				p_error$(OBJEXT) \
				p_first$(OBJEXT) \
				p_lalr_gen$(OBJEXT) \
				p_util$(OBJEXT) \
				p_string$(OBJEXT) \
				p_integrity$(OBJEXT) \
				p_virtual$(OBJEXT) \
				p_rewrite$(OBJEXT) \
				p_debug$(OBJEXT) \
				p_keywords$(OBJEXT) \
				p_build$(OBJEXT) \
				p_xml$(OBJEXT) \
				p_main$(OBJEXT)
				
HEADERS		=	p_global.h \
				p_error.h \
				p_defs.h \
				p_error.h
				
TEMPLATE_C	=	../Cparser/C.tlt
				
LIBS		=	-lphorward -lrt

MISCDEP		=	$(TEMPLATE_C) \
				Makefile

#-------------------------------------------------------------------------------

all: $(PROGRAM)
	@echo
	@echo --- Compilation succeeded! ---

$(TEMPLATE_C):
	cd ..;  cd Cparser; make C.tlt; cd ..; cd $(PROJECT)

$(PROG_BOOT1): $(PARSER_BOOT) $(PROTO) $(SRC) $(HEADERS) $(MISCDEP)
	min_lalr1 $(PARSER_BOOT) >$(PARSER) 2>$(PARSER_DBG)
	$(CC) $(CFLAGS) -o $@ $(DEBUG) -DUNICC_BOOTSTRAP=1 $(SRC) $(PARSER) $(LIBS)
	@echo
	@echo -- First bootstrap stage OK --
	@echo

$(PROG_BOOT2): $(PROG_BOOT1) $(PARSER_SRC) $(PROTO) $(SRC) $(HEADERS) $(MISCDEP)
	$(UNICC_ENV) ./$(PROG_BOOT1) -svwb $(PARSER_OUT) $(PARSER_SRC)
	$(CC) $(CFLAGS) -o $@ $(DEBUG) -DUNICC_BOOTSTRAP=2 $(SRC) $(PARSER) $(LIBS)
	@echo
	@echo -- Second bootstrap stage OK --
	@echo

$(PROG_BOOT3): $(PROG_BOOT2) $(PARSER_SRC) $(PROTO) $(SRC) $(HEADERS) $(MISCDEP)
	$(UNICC_ENV) ./$(PROG_BOOT2) -svwb $(PARSER_OUT) $(PARSER_SRC)
	$(CC) $(CFLAGS) -o $@ $(DEBUG) -DUNICC_BOOTSTRAP=3 $(SRC) $(PARSER) $(LIBS)
	@echo
	@echo -- Third bootstrap stage OK --
	@echo

$(PROGRAM): $(PROG_BOOT3) $(PARSER_SRC) $(PROTO) $(SRC) $(HEADERS) $(MISCDEP)
	$(UNICC_ENV) ./$(PROG_BOOT3) -svwb $(PARSER_OUT) $(PARSER_SRC)
	$(CC) $(CFLAGS) -o $@ $(DEBUG) $(SRC) $(PARSER) $(LIBS)
	@echo
	@echo -- Final bootstrap complete --
	@echo
	
clean: clean_obj
	-$(RM) $(PROGRAM)
	-$(RM) unicc.man
	-$(RM) README

clean_obj:
	-@$(RM) $(PROG_BOOT1)
	-@$(RM) $(PROG_BOOT2)
	-@$(RM) $(PROG_BOOT3)
	-@$(RM) $(PARSER)
	-@$(RM) $(PARSER_H)
	-@$(RM) $(PARSER_DBG)
	-@$(RM) $(PARSER_OBJ)
	-@$(RM) $(OBJ)

backup:	clean
	-@$(RM) ../p_lalr1_cc.tar
	tar cvf ../p_lalr1_cc.tar ../p_lalr1_cc

sourcelist: clean_obj
	@ls p_*.c p_*.h p_*.par p_*.syn

#Documentation related
doc: manpage README

manpage: unicc.man

README: unicc.t2t
	-$(RM) $@
	txt2tags -t txt -H -o - $? | sed -E -n '1h;1!H;$${;g;s/ +([-A-Z ]+)\n +(=+)/\2==\n \1 \n\2==/g;p;}' | sed -e "/^=/s/=/*/g;1,15d" >$@.tmp
	$(CAT) unicc.hdr $@.tmp >>$@
	$(RM) $@.tmp


unicc.man: unicc.t2t
	txt2tags -t man $?

# Install

INSTPATH ?= /usr
INSTPATH_BIN ?= $(INSTPATH)/bin
INSTPATH_MAN ?= $(INSTPATH)/share/man/man1
INSTPATH_SHR ?= $(INSTPATH)/share/unicc
INSTPATH_TLT ?= $(INSTPATH)/share/unicc/tlt

$(INSTPATH):
	test -d $(INSTPATH) || mkdir -p $(INSTPATH)

$(INSTPATH_BIN): $(INSTPATH)
	test -d $(INSTPATH_BIN) || mkdir -p $(INSTPATH_BIN)

$(INSTPATH_MAN): $(INSTPATH)
	test -d $(INSTPATH_MAN) || mkdir -p $(INSTPATH_MAN)

$(INSTPATH_SHR): $(INSTPATH)
	test -d $(INSTPATH_SHR) || mkdir -p $(INSTPATH_SHR)

$(INSTPATH_TLT): $(INSTPATH_SHR)
	test -d $(INSTPATH_TLT) || mkdir -p $(INSTPATH_TLT)

install: $(UNICC) unicc.man $(INSTPATH_BIN) $(INSTPATH_MAN) $(INSTPATH_TLT)
	cp $(UNICC) $(INSTPATH_BIN)
	gzip -c unicc.man >$(INSTPATH_MAN)/unicc.1.gz
	test -f $(UNICC_TPLDIR)/C.tlt && cp $(UNICC_TPLDIR)/C.tlt $(INSTPATH_TLT)

