CFLAGS    	= -g -I../phorward/src -DUTF8 -DUNICODE -DDEBUG -Wall $(CLOCAL)
LIBPHORWARD	= ../phorward/src/libphorward.a

HEADERS		= \
			unicc.h \
			proto.h

SOURCES   	=	\
			bnf.c \
			grammar.c \
			lr.c \
			parse.c

OBJECTS		= $(patsubst %.c,%.o,$(SOURCES))

all: unicc #ppgram2c

proto.h: $(SOURCES)
	pproto $(SOURCES) >$@

unicc: $(HEADERS) $(OBJECTS) main.o $(LIBPHORWARD)
	$(CC) -g -o $@ $(OBJECTS) main.o $(LIBPHORWARD)

ppgram2c: $(HEADERS) $(OBJECTS) ppgram2c.o $(LIBPHORWARD)
	$(CC) -g -o $@ $(OBJECTS) ppgram2c.o $(LIBPHORWARD)

test: unicc
	./unicc grammars/json.bnf grammars/test.json

clean:
	-rm $(OBJECTS)
	-rm unicc

