CFLAGS    	= -g -I../phorward/src -DDEBUG -DUTF8 -DUNICODE -Wall $(CLOCAL)
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

all: unicc gram2c bnftest

proto.h: $(SOURCES)
	pproto $(SOURCES) >$@

unicc: $(HEADERS) $(OBJECTS) main.o $(LIBPHORWARD)
	$(CC) -g -o $@ $(OBJECTS) main.o $(LIBPHORWARD)

gram2c: $(HEADERS) $(OBJECTS) gram2c.o $(LIBPHORWARD)
	$(CC) -g -o $@ $(OBJECTS) gram2c.o $(LIBPHORWARD)

bnftest.c: bnftestgen.c

bnftestgen.c: grammars/pbnf.bnf
	./gram2c -DPS $+ >$@

bnftest: gram2c $(HEADERS) $(OBJECTS) bnftest.o $(LIBPHORWARD)
	$(CC) -g -o $@ $(OBJECTS) bnftest.o $(LIBPHORWARD)

test: unicc
	./unicc grammars/json.bnf grammars/test.json

etareneg: gram2c
	awk -f etareneg.awk bnf.c >bnf.gen
	mv bnf.gen bnf.c

clean:
	-rm bnftestgen.c
	-rm $(OBJECTS)
	-rm *.o
	-rm unicc gram2c bnftest

