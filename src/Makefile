include ./Make.defines

PROGS = testcli testserv

all:	${PROGS}

	
testcli:	testcli.c
		${CC} ${CFLAGS} testcli.c -o ../bin/testcli ${LIBS}

testserv:	testserv.cpp
		g++ ${CFLAGS} testserv.cpp -o ../bin/testserv ${LIBS}


clean:
		rm -f ${PROGS} ${CLEANFILES}
