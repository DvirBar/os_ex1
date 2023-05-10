#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <exception>
#include <signal.h>
#include <time.h>
#include "Commands.h"
#include "Exception.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#define RET_VALUE_ERROR -1

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
      args[i] = (char *) malloc(s.length() + 1);
      memset(args[i], 0, s.length() + 1);
      strcpy(args[i], s.c_str());
      args[++i] = NULL;
  }

  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  // TODO: Uncomment
//  if (idx == string::npos) {
//    return;
//  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

SmallShell::SmallShell() {
    m_currPwd = getcwd(nullptr, 0);
    jobs = new JobsList;
    if(m_currPwd == string("")) {
        throw SyscallException("getcwd");
    }
    m_smashPrompt = "smash> ";
}

SmallShell::~SmallShell() {
    delete jobs;
}

string SmallShell::getCurrDir() const {
    return m_currPwd;
}

string SmallShell::getLastDir() const {
    return m_lastPwd;
}

void SmallShell::setCurrDir(const string& dir) {
    m_currPwd = dir;
}

void SmallShell::setLastDir(const string &dir) {
    m_lastPwd = dir;
}

JobsList *SmallShell::getJobsList() const {
    return jobs;
}

const std::string& SmallShell::getPrompt() const {
    return m_smashPrompt;
}

void SmallShell::setPrompt(const std::string &new_prompt) {
    m_smashPrompt = new_prompt;
}

Command::Command(const char *cmd_line):
    m_cmdLine(cmd_line)
{
    int numArgs = _parseCommandLine(cmd_line, m_args)-1;
    this->m_numArgs = numArgs;
}

Command::~Command() {
    for(int i=0; i<m_numArgs; i++) {
        ::free(m_args[i]);
    }
}

const char *Command::getCmdLine() const {
    return m_cmdLine;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line):
    Command(cmd_line)
{}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string commandStr = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (commandStr == "pwd") {
    return new GetCurrDirCommand(cmd_line);
  }

  else if (commandStr == "showpid") {
    return new ShowPidCommand(cmd_line);
  }

  else if(commandStr == "chprompt") {
      return new ChangePromptCommand(cmd_line);
  }

  else if(commandStr == "cd") {
      auto lastPwd = getInstance().getLastDir();
      auto currPwd = getInstance().getCurrDir();
      auto cdCom = new ChangeDirCommand(cmd_line, &lastPwd, &currPwd);
      setLastDir(lastPwd);
      setCurrDir(currPwd);
      return cdCom;
  }

  else if(commandStr == "fg") {
      return new ForegroundCommand(cmd_line, getJobsList());
  }

  else if(commandStr == "jobs") {
      return new JobsCommand(cmd_line, getJobsList());
  }

  else if(commandStr == "bg") {
      return new BackgroundCommand(cmd_line, getJobsList());
  }

  else if(commandStr == "quit") {
      return new QuitCommand(cmd_line, getJobsList());
  }

//  else if(commandStr == "kill") {
      return new KillCommand(cmd_line, getJobsList());
//  }

// return new ExternalCommand(cmd_line);

}

void SmallShell::executeCommand(const char *cmd_line) {
    jobs->removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);

    try {
        cmd->execute();
    }
    catch (const SyscallException& error) {
        ::perror(error.what());
    } catch(const exception& error) {
        cerr << error.what() << endl;
    }
    // Please note that you must fork smash process for some commands (e.g., external commands....)

    delete cmd;
}


ShowPidCommand::ShowPidCommand(const char *cmd_line):
        BuiltInCommand(cmd_line)
{}

void ShowPidCommand::execute() {
    int pid = getpid();
    cout << "smash pid is " << pid << endl;
}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line) :
        BuiltInCommand(cmd_line)
{
    if(m_numArgs == 0) {
        char cStringnewPropmpt[] = "smash> ";
        m_newPrompt = cStringnewPropmpt;
    }
    else
        m_newPrompt = m_args[1];
}

void ChangePromptCommand::execute() {
    std::string newPrompt = m_newPrompt;
    SmallShell::getInstance().setPrompt(newPrompt);
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) :
        BuiltInCommand(cmd_line)
{}

void GetCurrDirCommand::execute() {
    cout << SmallShell::getInstance().getCurrDir() << endl;
}


ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string* plastPwd, string* currPwd):
        BuiltInCommand(cmd_line)
{
    if(m_numArgs > MAX_ARGS) {
        throw TooManyArgsError();
    }

    // TODO: check for the same path
    // TODO: more exceptions?
    char* arg = m_args[1];

    if(arg == string("-")) {
        if (plastPwd == nullptr) {
            throw NoPWDError();
        }
        m_targetPwd = *plastPwd;
    } else {
        m_targetPwd = arg;
    }

    plastPwd = currPwd;
    *currPwd = m_targetPwd;
}

void ChangeDirCommand::execute() {
    int retValue = chdir(m_targetPwd.c_str());
    if(retValue == RET_VALUE_ERROR) {
        throw SyscallException("chdir");
    }
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) :
    BuiltInCommand(cmd_line)
{ }

void JobsCommand::execute() {
    SmallShell::getInstance().getJobsList()->printJobsList();
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs):
        BuiltInCommand(cmd_line)
{
    int jobId = 0;
    try {
        if(m_numArgs == Command::NO_ARGS) {
            if(jobs->isEmpty()) {
                throw JobsListIsEmptyError();
            }

            jobId = jobs->getMaxJobId();
        } else if(m_numArgs > ForegroundCommand::FG_MAX_NUM_ARGS) {
            throw FgInvalidArgumentsError();
        } else {
            jobId = stoi(m_args[1]);
        }

        m_job = jobs->getJobById(jobId);
    }

    catch(const invalid_argument& error) {
        throw FgInvalidArgumentsError();
    }

    catch(const JobNotFoundError& error) {
        throw FgJobNotFoundError(jobId);
    }
}

void ForegroundCommand::execute() {
    m_job->printCmdLine();

    if(m_job->isJobStopped()) {
        m_job->continueJob();
        if(kill(m_job->getPid(), SIGCONT) == RET_VALUE_ERROR) {
            throw SyscallException("kill");
        }
    }

    int status;
    if(waitpid(m_job->getPid(), &status, 0) == RET_VALUE_ERROR) {
        throw SyscallException("waitpid");
    }
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) :
        BuiltInCommand(cmd_line)
{
    int requestedJobID = 0;
    try {
        if(m_numArgs == Command::NO_ARGS) {
            m_bgJob = jobs->getLastStoppedJob();
        }
        else if (m_numArgs == BG_MAX_NUM_ARGS) {
            requestedJobID = stoi(m_args[1]);
            m_bgJob = jobs->getJobById(requestedJobID);
            if (!m_bgJob->isJobStopped())
                throw BgJobNotStoppedError(requestedJobID);
        }
        else
            throw BgInvalidArgumentsError();
    }

    catch (invalid_argument& invalidArgument) {
        throw BgInvalidArgumentsError();
    }

    catch (JobNotFoundError& jobNotFoundError) {
        throw BgJobNotFoundError(requestedJobID);
    }

    catch (NoStoppedJobs& noStoppedJobs) {
        throw BgNoStoppedJobs();
    }
}


void BackgroundCommand::execute() {
    m_bgJob->printCmdLine();
    m_bgJob->continueJob();
    if(kill(m_bgJob->getPid(), SIGCONT) == RET_VALUE_ERROR) {
        throw SyscallException("kill");
    }
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs):
    BuiltInCommand(cmd_line),
    execKill(false),
    m_jobs(jobs)
{
    if(m_args[1] == string("kill")) {
        execKill = true;
    }
}

void QuitCommand::execute() {
    if(execKill) {
        cout << "sending SIGKILL signal to 3 jobs:" << endl;
        m_jobs->killAllJobs();
    }

    ::exit(0);
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs):
    BuiltInCommand(cmd_line)
{
    int requestedJobID = 0;
    try {
        int requestedSig;
        if(m_numArgs != KILL_NUM_ARGS)
            throw KillInvalidArgumentsError();

        if(m_args[1][0] != KILL_SIGNAL_PREFIX)
            throw KillInvalidArgumentsError();

        std::string sigStr = m_args[1];
        requestedSig = stoi(sigStr.substr(1));
        requestedJobID = stoi(m_args[2]);

        m_killJob = jobs->getJobById(requestedJobID);
        m_sig = requestedSig;
    }

    catch (invalid_argument& invalidArgument) {
        throw KillInvalidArgumentsError();
    }

    catch (JobNotFoundError& jobNotFoundError) {
        throw KillJobNotFoundError(requestedJobID);
    }
}

void KillCommand::execute() {
    int pid = m_killJob->getPid();
    if(m_sig == SIGSTOP) {
        m_killJob->stopJob();
    }

    if(kill(pid, m_sig) == RET_VALUE_ERROR) {
        throw SyscallException("kill");
    }
    cout << "signal number " << m_sig << " was sent to pid " << pid << endl;
}



/* --------------------------------------------------- JOBS LIST ---------------------------------------------------- */
JobsList::~JobsList() {
    for(auto job: jobs) {
        delete job.second;
    }
}

void JobsList::addJob(Command *cmd, bool isStopped) {
    int jobId = assignJobId(jobs);
    auto jobEntry = new JobEntry(jobId, 0, cmd, isStopped);
    jobs.insert({jobId, jobEntry});
}

int JobsList::assignJobId(map<int, JobEntry*> jobs) {
    return jobs.rbegin()->first + 1;
}

void JobsList::removeJobById(int jobId) {
    JobEntry* jobEntry = getJobById(jobId);
    jobs.erase(jobId);
    delete jobEntry;
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    auto jobIterator = jobs.find(jobId);

    if(jobIterator == jobs.end()) {
        throw JobNotFoundError();
    }

    return jobIterator->second;
}

bool JobsList::isEmpty() const {
    return jobs.empty();
}

JobsList::JobEntry::JobEntry(int jobId, int jobPid, Command *cmd, bool isStopped) :
    m_jobId(jobId),
    m_pid(jobPid),
    m_cmd(cmd),
    m_isStopped(isStopped),
    m_insertTime(time(nullptr))
{ }

bool JobsList::JobEntry::isJobStopped() const {
    return m_isStopped;
}

int JobsList::JobEntry::getPid() const {
    return m_pid;
}

/*
 *  Print format:
 *  [<jobId>] <cmd_line> : <jobPid> {optional: <time> secs} {<optional: (stopped)>}
 * */
void JobsList::JobEntry::print( bool showStoppedFlag, bool includeTime) const {
    double timeDiff;
    string timeStr;
    time_t currTime = time(nullptr);

    if(includeTime) {
        timeDiff = difftime(m_insertTime, currTime);
        timeStr = " " + to_string(timeDiff) + " secs";
    }

    string stoppedFlag;
    if(m_isStopped && showStoppedFlag) {
        stoppedFlag = " (stopped)";
    }

    cout << "[" << m_jobId << "] " << m_cmd->getCmdLine() << " : " << m_pid << timeStr << stoppedFlag << endl;
}

void JobsList::JobEntry::printCmdLine() const {
    cout << m_cmd->getCmdLine() << " : " << m_pid << endl;
}

void JobsList::JobEntry::continueJob() {
    m_isStopped = false;
}

void JobsList::JobEntry::stopJob() {
    m_isStopped = true;
}

int JobsList::getMaxJobId() const {
    if(jobs.empty()) {
        throw JobsListIsEmptyError();
    }

    return jobs.rbegin()->first;
}

void JobsList::removeFinishedJobs() {
    int status;
    int waitpidCheck;
    for (auto job: jobs) {
        waitpidCheck = waitpid(job.second->getPid(), &status, WNOHANG);

        if(waitpidCheck > 0) {
            removeJobById(job.first);
        }

        else if(waitpidCheck == RET_VALUE_ERROR) {
            throw SyscallException("waitpid");
        }
    }
}

void JobsList::printJobsList() {
    removeFinishedJobs();
    for(auto job: jobs){
        job.second->print(true, true);
    }
}

JobsList::JobEntry*  JobsList::getLastStoppedJob() {
    JobEntry* jobEntry = jobs.begin()->second;
    for(auto job: jobs) {
        if(job.second->isJobStopped())
            jobEntry = job.second;
    }
    if(!jobEntry->isJobStopped()) {
        throw NoStoppedJobs();
    }

    return  jobEntry;

}

void JobsList::killAllJobs() {
    for(auto job: jobs) {
        job.second->print(false, false);
        kill(job.second->getPid(), SIGKILL);
        removeJobById(job.first);
    }
}
