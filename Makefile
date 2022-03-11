.POSIX:

CC      = cc
CFLAGS  = -ansi -pipe -O3 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -pedantic
LDFLAGS = -ltask

binaries = mktask

all: ${binaries}

mktask: src/mktask.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ src/mktask.c

clean:
	rm -f ${binaries}
