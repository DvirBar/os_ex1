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
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
    m_currPwd = getcwd(nullptr, 0);
    m_smashPrompt = "smash> ";
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

const char *SmallShell::getCurrDir() const {
    return m_currPwd;
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
    char* args[Command::CMD_MAX_NUM_ARGS+1];
    int numArgs = _parseCommandLine(cmd_line, args)-1;

    this->m_args = args;
    this->m_numArgs = numArgs;
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

  if (commandStr.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }

  else if (commandStr.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }

  else if(commandStr.compare("chprompt") == 0) {
      return new ChangePromptCommand(cmd_line);
  }

  else if(commandStr.compare("cd")) {
      return SmallShell::handleChdirCommand(&m_lastPwdList, &m_currPwd, cmd_line);
  }

  else if(commandStr.compare(("fg"))) {
      return new ForegroundCommand(cmd_line, getJobsList());
  }

 return new ExternalCommand(cmd_line);

}

Command* SmallShell::handleChdirCommand(stack<char *> *lastPwdList, char **currPwd, const char *cmd_line) {
    char** plastPwd = nullptr;
    char* lastPwdCopy = nullptr;

    if(!lastPwdList->empty()) {
        plastPwd = &(lastPwdList->top());
        lastPwdCopy = lastPwdList->top();
    }

    auto cdCom = new ChangeDirCommand(cmd_line, plastPwd);

    // If plastPwd wasn't changed, it means that we used "-" argument
    if(*plastPwd == lastPwdCopy) {
        lastPwdList->pop();
    } else {
        lastPwdList->push(*plastPwd);
        *(currPwd) = *plastPwd;
    }

    return cdCom;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Manage job list

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
    if(m_numArgs == 1) {
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


ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd):
        BuiltInCommand(cmd_line)
{
    if(m_numArgs > MAX_ARGS) {
        throw TooManyArgsError();
    }

    // TODO: check for invalid literals
    // TODO: check for the same path
    // TODO: more exceptions?

    char* arg = m_args[1];
    if(arg == string("-") && plastPwd == nullptr) {
        throw NoPWDError();
    }

    m_lastPwd = arg;
    *plastPwd = arg;
}

void ChangeDirCommand::execute() {
    int retValue = chdir(m_lastPwd);
    if(retValue == RET_VALUE_ERROR) {
        throw SyscallChdirError();
    }
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs):
        BuiltInCommand(cmd_line)
{
    int jobId = stoi(m_args[1]);

    if(m_numArgs == Command::NO_ARGS) {
        jobId = jobs->getMaxJobId();
    } else if(m_numArgs > ForegroundCommand::FG_MAX_NUM_ARGS) {
        throw FgInvalidArgumentsError();
    }

    m_job = jobs->getJobById(jobId);
}

void ForegroundCommand::execute() {
    m_job->print();

    if(m_job->isJobStopped()) {
        kill(m_job->getPid(), SIGCONT);
    }

    int status;
    waitpid(m_job->getPid(), &status, 0);
}

// JOBS
void JobsList::addJob(Command *cmd, bool isStopped) {
    int jobId = assignJobId(jobs);
    auto jobEntry = new JobEntry(jobId, cmd, isStopped);
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
        throw JobNotFoundError(jobId);
    }

    return jobIterator->second;
}

JobsList::JobEntry::JobEntry(int jobId, Command *cmd, bool isStopped):
    m_jobId(jobId),
    m_cmd(cmd),
    m_isStopped(isStopped),
    m_insertTime(time(nullptr))
{
    // TODO: how to get job pid?
}

void JobsList::JobEntry::print(bool includeTime) const {
    double timeDiff;
    string timeStr;
    time_t currTime = time(nullptr);

    if(includeTime) {
        timeDiff = difftime(m_insertTime, currTime);
        timeStr = to_string(timeDiff);
    }

    string stoppedFlag;
    if(m_isStopped) {
        stoppedFlag = " (stopped)";
    }

    cout << "[" << m_jobId << "] " << m_cmd->getCmdLine() << " : " << m_pid << " "
    << timeStr << " secs"<< stoppedFlag << endl;
}

int JobsList::getMaxJobId() const {
    if(jobs.empty()) {
        throw JobsListIsEmptyError();
    }

    return jobs.rbegin()->first;
}
