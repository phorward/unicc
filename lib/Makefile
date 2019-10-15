LIBPHORWARD = libphorward.a
PREGEX = pregex
PLEX = plex

CFLAGS += -g -DDEBUG -DUNICODE -DUTF8 -I. -Wall
LDLIBS += libphorward.a

SRC = \
	$(PREFIX)util.c \
	$(PREFIX)pregex.c \
	$(PREFIX)plex.c \
	$(PREFIX)phorward.c \


OBJ = $(patsubst %.c,%.o,$(SRC))

all: $(LIBPHORWARD) $(PREGEX) $(PLEX)

$(LIBPHORWARD): phorward.h $(OBJ)
	$(AR) rv $@ $(OBJ)

clean:
	-rm -f $(OBJ)
	-rm -f $(LIBPHORWARD) $(PREGEX) $(PLEX)

