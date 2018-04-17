OUTPUT		= 	../c++.tlt
BASE		=	C++.xml

MKPARSER	=	../tools/mkparser.awk

SOURCE		=	\
				defines.h \
				fn.ast.cpp \
				fn.clearin.cpp \
				fn.debug.cpp \
				fn.getact.cpp \
				fn.getgo.cpp \
				fn.getinput.cpp \
				fn.getsym.cpp \
				fn.handleerr.cpp \
				fn.lex.cpp \
				fn.main.cpp \
				fn.parse.cpp \
				fn.stack.cpp \
				fn.unicode.cpp \
				ast.h \
				parser.h \
				prodinfo.h \
				syminfo.h \
				token.h

#-------------------------------------------------------------------------------

all: $(OUTPUT)

$(OUTPUT): $(SOURCE) $(BASE) $(MKPARSER)
	awk -f $(MKPARSER) $(BASE) >$@
	@echo C++ target generated

clean:
	-$(RM) $(OUTPUT)

