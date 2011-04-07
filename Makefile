#-------------------------------------------------------------------------------
# UniCC LALR(1) Parser Generator
# Copyright (C) 2006-2009 by Phorward Software Technologies, Jan Max Meyer
# http://unicc.phorward-software.com/ ++ unicc<<AT>>phorward-software<<DOT>>com
#
# Make-File:	Makefile
# Author:		Jan Max Meyer
#
# You may use, modify and distribute this software under the terms and condi-
# tions of the Artistic License, version 2. Please see LICENSE for more infor-
# mation.
#-------------------------------------------------------------------------------

include		../../include/Make.inc

PROGRAM		=	$(RUN_DIR)$(PATH_SEP)$(UNICC)$(EXEEXT)
PROG_BOOT1	=	boot_unicc1$(EXEEXT)
PROG_BOOT2	=	boot_unicc2$(EXEEXT)

PARSER		=	p_parse.c
PARSER_OBJ	=	p_parse$(OBJEXT)

PARSER_BOOT	=	p_parse.syn
PARSER_SRC	=	p_parse.par
PARSER_DBG	=	p_parse.dbg

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
				p_main$(OBJEXT)
				
HEADERS		=	p_global.h \
				p_error.h \
				p_proto.h \
				p_defs.h \
				p_error.h
				
LIBS		=	$(PBASIS_LIB) \
				$(PREGEX_LIB) \
				$(PSTRING_LIB) \
				$(XML_LIB)

#-------------------------------------------------------------------------------

all: $(PROGRAM)
	@echo
	@echo --- Compilation succeeded! ---

$(PROG_BOOT1): $(PARSER_BOOT) $(SRC) $(HEADERS) $(LIBS) Makefile
	$(MIN_LALR1) $(PARSER_BOOT) >$(PARSER) 2>$(PARSER_DBG)
	$(CC) $(CEXEOPTS) $(SRC) $(PARSER)
	$(LLINK) $(LINKOPTS)$@ $(OBJ) $(PARSER_OBJ) $(LIBS)
	@echo
	@echo -- First bootstrap stage OK --
	@echo

$(PROG_BOOT2): $(PROG_BOOT1) $(PARSER_SRC) $(SRC) $(HEADERS) $(LIBS) Makefile
	$(PROG_BOOT1) -v -w -o $(PARSER) $(PARSER_SRC)
	$(CC) $(CEXEOPTS) $(SRC) $(PARSER)
	$(LLINK) $(LINKOPTS)$@ $(OBJ) $(PARSER_OBJ) $(LIBS)
	@echo
	@echo -- Second bootstrap stage OK --
	@echo

$(PROGRAM): $(PROG_BOOT2) $(PARSER_SRC) $(SRC) $(HEADERS) $(LIBS) Makefile
	$(PROG_BOOT2) -v -w -o $(PARSER) $(PARSER_SRC)
	$(CC) $(CEXEOPTS) $(SRC) $(PARSER)
	$(LLINK) $(LINKOPTS)$@ $(OBJ) $(PARSER_OBJ) $(LIBS)
	@echo
	@echo -- Final bootstrap complete --
	@echo

clean: clean_obj
	-@$(RM) $(PROGRAM)

clean_obj:
	-@$(RM) $(PROG_BOOT1)
	-@$(RM) $(PROG_BOOT2)
	-@$(RM) $(PARSER)
	-@$(RM) $(PARSER_DBG)
	-@$(RM) $(PARSER_OBJ)
	-@$(RM) $(OBJ)

backup:
	-@$(RM) ../p_lalr1_cc.tar
	tar cvf ../p_lalr1_cc.tar ../p_lalr1_cc

sourcelist: clean_obj
	@ls p_*.c p_*.h p_*.par p_*.syn

