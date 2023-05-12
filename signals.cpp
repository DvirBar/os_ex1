#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
}

void ctrlCHandler(int sig_num) {
  cout << "ctrl-C" << endl;
  pid_t pid = getpid();
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

