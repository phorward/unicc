XPL				= 	xpl

XPL_PARFILE		= xpl.par
XPL_PARSER		= xpl.parser
XPL_PARSER_C	= $(XPL_PARSER).c
XPL_PARSER_H	= $(XPL_PARSER).h
XPL_PARSER_O	= $(XPL_PARSER).o

XPL_SOURCE		= 	\
					xpl.debug.c \
					xpl.functions.c \
					xpl.main.c \
					xpl.program.c \
					xpl.run.c \
					xpl.util.c \
					xpl.value.c \
					$(XPL_PARSER_C)

XPL_OBJECTS		= 	\
					xpl.debug.o \
					xpl.functions.o \
					xpl.main.o \
					xpl.program.o \
					xpl.run.o \
					xpl.util.o \
					xpl.value.o \
					$(XPL_PARSER_O)


all: $(XPL)

$(XPL): $(XPL_SOURCE) Makefile
	$(CC) -o $@ $(XPL_SOURCE)

$(XPL_PARSER_C): $(XPL_PARFILE)
	unicc -svwb $(XPL_PARSER) $(XPL_PARFILE)

clean:
	rm -f $(XPL_PARSER_C)
	rm -f $(XPL_PARSER_H)
	rm -f $(XPL_OBJECTS)
	rm -f $(XPL)
