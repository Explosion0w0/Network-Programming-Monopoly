include ./Make.defines

PROGS = testcli testserv #client server

all:	${PROGS}


#client:	client.o
#		${CC} ${CFLAGS} -o $@ client.o ${LIBS}

#server:	server.o
#		${CC} ${CFLAGS} -o $@ server.o ${LIBS}
		
		
testcli:	testcli.o
		${CC} ${CFLAGS} -o $@ testcli.o ${LIBS}

testserv:	testserv.cpp
		g++ ${CFLAGS} -o testserv testserv.cpp ${LIBS}


clean:
		rm -f ${PROGS} ${CLEANFILES}
