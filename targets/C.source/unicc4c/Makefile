#-------------------------------------------------------------------------------
# NEWPROJECT
# Copyright (C) 2010 by Phorward Software Technologies, Jan Max Meyer
# http://www.phorward-software.com ++ contact<AT>phorward<DASH>software<DOT>com
#
# Make-File:	Makefile
# Author:		Jan Max Meyer
#-------------------------------------------------------------------------------

include			../../include/Make.inc

PROJECT		=	unicc4c

PROGRAM		=	$(PROJECT)$(EXEEXT)

PARSER_C	=	PARSER.c

SOURCE		=	\
				gen.c \
				gen.act.c \
				gen.go.c \
				gen.lex.c \
				gen.misc.c \
				gen.prod.c \
				main.c \
				output.c \
				util.c
				

OBJECTS		=	\
				gen$(OBJEXT) \
				gen.act$(OBJEXT) \
				gen.go$(OBJEXT) \
				gen.lex$(OBJEXT) \
				gen.misc$(OBJEXT) \
				gen.prod$(OBJEXT) \
				main$(OBJEXT) \
				output$(OBJEXT) \
				util$(OBJEXT)

LIBS		=	$(PSTRING_LIB) \
				$(PREGEX_LIB) \
				$(PBASIS_LIB)

PROTOFILE	=	proto.h
				
#-------------------------------------------------------------------------------

all: $(PROGRAM)
	@echo
	@echo --- Compilation succeeded! ---
	
$(PROGRAM): $(PROTOFILE) $(OBJECTS) $(SOURCE) $(LIBS) Makefile
	$(LLINK) $(LINKOPTS)$@ $(OBJECTS) $(LIBS) $(LIBNAME)

$(OBJECTS): $(SOURCE) $(PARSER_C)
	$(CC) $(CEXEOPTS) $(DEBUG) $(SOURCE)

$(PARSER_C):
	cd ..;  cd Cparser; make ../$(PROJECT)/$@; cd ..; cd $(PROJECT)

$(PROTOFILE): $(SOURCE)
	cproto_all "$(SOURCE)" $(INCLUDE_DIR) $(PROTOFILE)

clean_obj:
	-$(RM) $(OBJECTS)

clean: clean_obj
	-$(RM) $(PROGRAM)
	-$(RM) $(PROTOFILE)
	-$(RM) $(PARSER_C)

backup: clean
	-@$(RM) ../$(PROJECT).tar
	tar cvf ../$(PROJECT).tar ../$(PROJECT)

