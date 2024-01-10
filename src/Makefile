include ./Make.defines

PROGS = gameboard testcli gameboard.o # testcli testserv

all:	${PROGS}


gameboard:	gameboard.cpp
		g++ ${CFLAGS} gameboard.cpp -o ../bin/gameboard ${LIBS}

gameboard.o:	gameboard.cpp
		g++ ${CFLAGS} gameboard.cpp -o ../bin/gameboard.o ${LIBS}

testcli:	testcli.cpp
		g++  ${CFLAGS} testcli.cpp `wx-config --cxxflags --libs std` -o ../bin/testcli ${LIBS}

	
#testcli:	testcli.c
#		${CC} ${CFLAGS} testcli.c -o ../bin/testcli ${LIBS}

#testserv:	testserv.cpp
#		g++ ${CFLAGS} testserv.cpp -o ../bin/testserv ${LIBS}


clean:
		rm -f ${PROGS} ${CLEANFILES}
