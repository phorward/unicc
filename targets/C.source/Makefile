C_XML		= 	../c.tlt
C_XML_SRC	=	STDTPL.xml

#PARSER_C	=	../unicc4c/PARSER.c
#PARSER_C_SRC=	UNICC4C.c

MKPARSER	=	mkparser.awk

SOURCE		=	\
				defines.h \
				fn.ast.c \
				fn.clearin.c \
				fn.debug.c \
				fn.getact.c \
				fn.getchar.c \
				fn.getgo.c \
				fn.getinput.c \
				fn.getsym.c \
				fn.handleerr.c \
				fn.lex.c \
				fn.main.c \
				fn.parse.c \
				fn.syntree.c \
				fn.stack.c \
				fn.unicode.c \
				ast.h \
				pcb.h \
				prodinfo.h \
				syminfo.h \
				syntree.h \
				tables.h \
				token.h

#-------------------------------------------------------------------------------

#all: $(C_XML) $(PARSER_C)
all: $(C_XML)

$(C_XML): $(SOURCE) $(C_XML_SRC) $(MKPARSER)
	awk -v genwhat=STDTPL -f $(MKPARSER) $(C_XML_SRC) >$@
	@echo XML standard template generated

#$(PARSER_C): $(SOURCE) $(PARSER_C_SRC) $(MKPARSER)
#	$(AWK) -v genwhat=UNICC4C -f $(MKPARSER) $(PARSER_C_SRC) >$@
#	@echo UniCC4C skeleton program generated
	
clean:
	-$(RM) $(C_XML)
#	-$(RM) $(PARSER_C)

