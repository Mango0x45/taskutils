.POSIX:

CC      = cc
CFLAGS  = -ansi -pipe -O3 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -pedantic
LDFLAGS = -ltask
PREFIX  = /usr

binaries = mktask

all: ${binaries}

debug:
	make 'CFLAGS+=-DDEBUG -g -ggdb -Og'

mktask: src/mktask.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ src/mktask.c

install:
	mkdir -p ${PREFIX}/bin
	cp ${binaries} ${PREFIX}/bin

clean:
	rm -f ${binaries}
