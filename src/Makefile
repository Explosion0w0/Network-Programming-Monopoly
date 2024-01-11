include ./Make.defines

PROGS = monopoly_client monopoly_server

all:	${PROGS}


monopoly_server:	monopoly_server.cpp
		g++ ${CFLAGS} monopoly_server.cpp -o ../bin/monopoly_server ${LIBS}


monopoly_client:	monopoly_client.cpp
		g++  ${CFLAGS} monopoly_client.cpp `wx-config --cxxflags --libs std` -o ../bin/monopoly_client ${LIBS}

	

clean:
		rm -f ${PROGS} ${CLEANFILES}
