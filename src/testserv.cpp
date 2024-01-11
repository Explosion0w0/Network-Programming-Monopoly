#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <chrono>
#include <thread>

extern "C" { 
#include "unp.h"
} 

using namespace std;


static void sig_alrm(int signo) {
    cout << "timeout\n";
	return;
}

void
sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	pid = wait(&stat);
	printf("child %d terminated\n", pid);
	return;
}

int
main(int argc, char **argv) {
	fd_set rset;
    FD_ZERO(&rset);
	Signal(SIGALRM, sig_alrm);
	int maxfd = STDIN_FILENO + 1;

	for ( ; ; ) {
		timeval tv = {10,0};
		FD_SET(STDIN_FILENO, &rset);
		alarm(3);
		int n = select(maxfd, &rset, NULL, NULL, &tv);
		if (FD_ISSET(STDIN_FILENO, &rset)) {
			cin.ignore(3);
		}
		cout << n << "\n";
		if (n <= 0) {
			if (errno == ETIMEDOUT) {
				cout << "ETIMEDOUT\n";
			} else {
				cout << errno << "\n";
			}
		} else {
			cout << "\n";
		}

		alarm(0);
	}
}
