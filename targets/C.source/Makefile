C_XML		= 	../c.tlt
C_XML_SRC	=	C.xml

MKPARSER	=	../tools/mkparser.awk

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

all: $(C_XML)

$(C_XML): $(SOURCE) $(C_XML_SRC) $(MKPARSER)
	awk -f $(MKPARSER) $(C_XML_SRC) >$@
	@echo C target generated

clean:
	-$(RM) $(C_XML)

