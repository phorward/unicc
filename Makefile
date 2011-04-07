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

PCC			=	unicc
PROGRAM		=	$(RUN_DIR)$(PATH_SEP)$(PCC)$(EXEEXT)
PROGRAM2	=	$(RUN_DIR)$(PATH_SEP)$(PCC)1$(EXEEXT)
PROGRAM3	=	$(RUN_DIR)$(PATH_SEP)$(PCC)2$(EXEEXT)

PARSER		=	p_parse.c

PARSER2		=	p_parse1.c

PARSER3		=	p_parse2.c

PARSER_SRC	=	p_parse.syn

PARSER2_SRC	=	p_parse.par

PARSER3_SRC	=	p_parse.par

PARSER_DBG	=	p_parse.dbg

SRC			=	p_mem.c \
				p_error.c \
				p_parse.c \
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
				
SRC2		=	p_mem.c \
				p_error.c \
				p_parse1.c \
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
				
SRC3		=	p_mem.c \
				p_error.c \
				p_parse2.c \
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
				p_parse$(OBJEXT) \
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
				
OBJ2		=	p_mem$(OBJEXT) \
				p_error$(OBJEXT) \
				p_parse1$(OBJEXT) \
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
				
OBJ3		=	p_mem$(OBJEXT) \
				p_error$(OBJEXT) \
				p_parse2$(OBJEXT) \
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

final: all
	$(RM) $(PROGRAM)
	$(RM) $(PROGRAM2)
	$(MV) $(PROGRAM3) $(PROGRAM)
	@echo
	@echo --- Final compilation succeeded! ---

all: $(PROGRAM) $(PROGRAM2) $(PROGRAM3)
	@echo
	@echo --- Compilation succeeded! ---

parser: $(PARSER)

$(PARSER): $(PARSER_SRC)
	$(MIN_LALR1) $(PARSER_SRC) >$@ 2>$(PARSER_DBG)
	
$(PARSER2): $(PARSER2_SRC)
	$(PCC) -v -w -o $@ $(PARSER2_SRC)
	
$(PARSER3): $(PARSER3_SRC)
	$(PCC)1 -v -w -o $@ $(PARSER3_SRC)

$(PROGRAM): $(PARSER) $(SRC) $(HEADERS) $(LIBS) Makefile
	$(CC) $(CEXEOPTS) $(SRC)
	$(LLINK) $(LINKOPTS)$@ $(OBJ) $(LIBS)
	@echo
	@echo -- Bootstrap stage 1 OK --
	@echo

$(PROGRAM2): $(PARSER2) $(SRC2) $(HEADERS) $(LIBS) Makefile
	$(CC) $(CEXEOPTS) $(SRC2)
	$(LLINK) $(LINKOPTS)$@ $(OBJ2) $(LIBS)
	@echo
	@echo -- Bootstrap stage 2 OK --
	@echo

$(PROGRAM3): $(PARSER3) $(SRC3) $(HEADERS) $(LIBS) Makefile
	$(CC) $(CEXEOPTS) $(SRC3)
	$(LLINK) $(LINKOPTS)$@ $(OBJ3) $(LIBS)
	@echo
	@echo -- Bootstrap stage 3 OK --
	@echo

clean: clean_obj
	-@$(RM) $(PROGRAM)
	-@$(RM) $(PROGRAM2)
	-@$(RM) $(PROGRAM3)

clean_obj:
	-@$(RM) $(PARSER_DBG)
	-@$(RM) $(PARSER)
	-@$(RM) $(PARSER2)
	-@$(RM) $(PARSER3)
	-@$(RM) *$(OBJEXT)
	-@$(RM) *.ncb
	-@$(RM) muell
	-@$(RM) *.tmp

backup:
	-@$(RM) ../p_lalr1_cc.tar
	tar cvf ../p_lalr1_cc.tar ../p_lalr1_cc

sourcelist: clean_obj
	@ls p_*.c p_*.h p_*.par p_*.syn

