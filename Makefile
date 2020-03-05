PREFIX		?= /usr

CFLAGS    	= -g -DDEBUG -DUTF8 -DUNICODE -Wall $(CLOCAL)

HEADERS		= \
			lib/phorward.h \
			unicc.h \
			proto.h

PHORWARD	= \
			lib/phorward.c

SOURCES   	=	\
			bnf.c \
			grammar.c \
			lr.c \
			parse.c

OBJECTS		= $(patsubst %.c,%.o,$(PHORWARD)) $(patsubst %.c,%.o,$(SOURCES))

all: unicc gram2c

proto.h: $(SOURCES)
	lib/pproto $(SOURCES) >$@

bnf.c: grammars/pbnf.bnf
	awk -f etareneg.awk bnf.c >bnf.gen
	mv bnf.gen bnf.c

unicc: $(HEADERS) $(OBJECTS) main.o $(LIBPHORWARD)
	$(CC) -g -o $@ $(OBJECTS) main.o $(LIBPHORWARD)

gram2c: $(HEADERS) $(OBJECTS) gram2c.o $(LIBPHORWARD)
	$(CC) -g -o $@ $(OBJECTS) gram2c.o $(LIBPHORWARD)

#bnftest.c: bnftestgen.c

#bnftestgen.c: grammars/pbnf.bnf
#	./gram2c -DPS $+ >$@

#bnftest: gram2c $(HEADERS) $(OBJECTS) bnftest.o $(LIBPHORWARD)
#	$(CC) -g -o $@ $(OBJECTS) bnftest.o $(LIBPHORWARD)

test: unicc
	./unicc grammars/json.bnf grammars/test.json

clean:
	-rm bnftestgen.c
	-rm $(OBJECTS)
	-rm *.o
	-rm unicc gram2c

install: unicc
	cp unicc $(PREFIX)/bin/unicc

