include ./Make.defines

PROGS = testcli testserv

all:	${PROGS}

	
testcli:	testcli.c
		${CC} ${CFLAGS} testcli.c -o testcli ${LIBS}

testserv:	testserv.cpp
		g++ ${CFLAGS} testserv.cpp -o testserv ${LIBS}


clean:
		rm -f ${PROGS} ${CLEANFILES}
