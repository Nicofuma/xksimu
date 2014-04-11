all:

INSTALL_PATH = /usr/
CC = gcc
PEDANTIC_PARANOID_FREAK =       -Wall -Wshadow -Wcast-align \
				-Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
				-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
				-Wmissing-noreturn  \
				-Wpointer-arith -Wwrite-strings -finline-functions
# -Wredundant-decls
REASONABLY_CAREFUL_DUDE =	-Wall
NO_PRAYER_FOR_THE_WICKED =	-w
WARNINGS = 			$(PEDANTIC_PARANOID_FREAK)
CFLAGS = -g -O0 $(WARNINGS) -std=c99

INCLUDES = 
DEFS = -I$(INSTALL_PATH)/include -L$(INSTALL_PATH)/lib/
LDADD = -lm -lsimgrid 
LIBS = 

C_FILES = $(wildcard *.c)
BIN_FILES = $(patsubst %.c,%,$(C_FILES))
O_FILES = $(patsubst %.c,%.o,$(C_FILES))

%: %.o list.o commons.o
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@ 

%.o: %.c
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<

all: xksimu

clean:
	rm -f $(BIN_FILES) *.o *~
.SUFFIXES:
.PHONY : clean
