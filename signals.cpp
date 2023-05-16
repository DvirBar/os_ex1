#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>
#include "Exception.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	cout << "smash: got ctrl-Z" << endl;
    auto foreJob = SmallShell::getInstance().getForegroundJob();
    if(foreJob != nullptr) {
        pid_t runningPid = foreJob->getPid();
        if(kill(runningPid, SIGSTOP) == SmallShell::RET_VALUE_ERROR) {
            throw SyscallException("kill");
        }
        SmallShell::getInstance().getJobsList()->addJob(foreJob->getCmdLine().c_str(), runningPid, foreJob->getJobId(),
                                                        true, foreJob->getTimeout());
        SmallShell::getInstance().removeForegroundJob();
        cout << "smash: process " << runningPid << " was stopped" << endl;
    }

}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;

    auto foreJob = SmallShell::getInstance().getForegroundJob();
    if(foreJob != nullptr) {
        pid_t runningPid = foreJob->getPid();
        // TODO: is it being caught?
        if(kill(runningPid, SIGKILL) == SmallShell::RET_VALUE_ERROR) {
            throw SyscallException("kill");
        }
        SmallShell::getInstance().removeForegroundJob();

        cout << "smash: process " << runningPid << " was killed" << endl;
    }
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm" << endl;
    JobsList::JobEntry* foreJob = SmallShell::getInstance().getForegroundJob();
    if(foreJob != nullptr && foreJob->isTimedOut()) {
        if(kill(foreJob->getPid(), SIGKILL) == SmallShell::RET_VALUE_ERROR) {
            throw SyscallException("kill");
        }
        cout << "smash: " + foreJob->getCmdLine() + " timed out!" << endl;
        SmallShell::getInstance().removeForegroundJob();
    }

    JobsList* jobsList = SmallShell::getInstance().getJobsList();
    jobsList->terminateTimedOutJobs();
    SmallShell::getInstance().refreshTimeout();
}

