.POSIX:

CC      = cc
CFLAGS  = -ansi -pipe -O3 -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -pedantic
LDFLAGS = -ltask
PREFIX  = /usr

binaries = addtask mktask rmtask
mandir   = man

all: ${binaries}

debug:
	make 'CFLAGS+=-DDEBUG -g -ggdb -Og'

addtask: src/addtask.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ src/addtask.c

mktask: src/mktask.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ src/mktask.c

rmtask: src/rmtask.c
	${CC} ${CFLAGS} -o $@ src/rmtask.c

install:
	mkdir -p ${PREFIX}/bin ${PREFIX}/share/man/man1
	cp ${binaries} ${PREFIX}/bin
	cp ${mandir}/*.1 ${PREFIX}/share/man/man1

clean:
	rm -f ${binaries}
