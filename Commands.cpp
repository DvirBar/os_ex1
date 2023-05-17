#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sched.h>
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

/* --------------------------------------------------- String Commands ---------------------------------------------------- */

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

int _parseCommandLine(const char* cmd_line, char** args, bool isTimeout = false) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  int idleIndex = 0;
  for(std::string s; iss >> s; ) {
      if(!isTimeout || (isTimeout && idleIndex == 2)) {
          args[i] = (char *) malloc(s.length() + 1);
          memset(args[i], 0, s.length() + 1);
          strcpy(args[i], s.c_str());
          args[++i] = NULL;
      } else {
          idleIndex++;
      }
  }

  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

string _removeBackgroundSign(string cmd_line) {
  // find last character other than spaces
  ::size_t idx = cmd_line.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return

  if (idx == string::npos) {
    return cmd_line;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return cmd_line;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
//  // truncate the command line string up to the last non-space character
//  cmd_line[cmd_line.find_last_not_of(WHITESPACE, idx) + 1] = 0;

    return cmd_line;
}
/* ------------------------------------------ Command Smash and Built in ------------------------------------------- */
SmallShell::SmallShell():
    m_foregroundJob(nullptr),
    m_minTimeout(nullptr),
    m_timeoutCounter(0)
{
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
    return getcwd(nullptr, 0);
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

JobsList::JobEntry* SmallShell::getForegroundJob() const {
    return m_foregroundJob;
}

void SmallShell::setForegroundJob(JobsList::JobEntry *jobEntry) {
    m_foregroundJob = jobEntry;
}

void SmallShell::removeForegroundJob() {
    delete m_foregroundJob;
    m_foregroundJob = nullptr;
}

void SmallShell::addTimeout(int timeoutSecs) {
    m_timeoutCounter++;
    auto timeout = new Timeout(m_timeoutCounter, timeoutSecs);
    if(m_minTimeout == nullptr) {
        m_minTimeout = timeout;
        alarm(timeoutSecs);
        return;
    }

    if(timeoutSecs == m_minTimeout->getDiff()) {
        return;
    }

    if(timeoutSecs < m_minTimeout->getDiff()) {
        timeoutMap.insert({m_minTimeout->getId(), m_minTimeout});
        m_minTimeout = timeout;
        alarm(timeoutSecs);
        return;
    }

    for(auto timeoutItem: timeoutMap) {
        if(timeoutItem.second->getDiff() == timeoutSecs) {
            return;
        }
    }

    timeoutMap.insert({m_timeoutCounter, timeout});
}

void SmallShell::refreshTimeout() {
    int minDiff = -1;
    Timeout* minTimeout = nullptr;
    for(auto timeout: timeoutMap) {
        int diff = timeout.second->getDiff();
        if(minDiff == -1 ||  diff < minDiff) {
            minDiff = diff;
            minTimeout = timeout.second;
        }
    }

    if(minTimeout != nullptr) {
        timeoutMap.erase(minTimeout->getId());
        delete m_minTimeout;
        alarm(minTimeout->getDiff());
    }

    m_minTimeout = minTimeout;
}

Command::Command(const char *cmd_line, bool skipBgRemove):
    m_rawCmdLine(cmd_line)
{
    string cmdLineCopy;
    if(skipBgRemove) {
        cmdLineCopy = _trim(cmd_line);
    } else {
        cmdLineCopy = _trim(_removeBackgroundSign(cmd_line));
    }

    string cmd_s = _trim(_removeBackgroundSign(cmd_line));
    string commandStr = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    bool isTimeout = false;
    if(commandStr== "timeout") {
        isTimeout = true;
    }

    m_cmdLine = cmdLineCopy.c_str();
    int numArgs = _parseCommandLine(m_cmdLine, m_args, isTimeout)-1;
    this->m_numArgs = numArgs;

    if(hasBackgroundSign(string(cmd_line))) {
        m_isBackground = true;
    } else {
        m_isBackground = false;
    }
}

Command::~Command() {
    for(int i=0; i<m_numArgs; i++) {
        ::free(m_args[i]);
    }
}

void Command::splitCommand(const string& str, const string& delimiter, string commands[2]) {
    commands[0] = _trim(str.substr(0, str.find(delimiter)));
    commands[1] = _trim(str.substr(commands[0].length() + delimiter.length()+1, str.length()));
}

bool Command::hasBackgroundSign(string cmd_line) {
    ::size_t idx = cmd_line.find_last_not_of(WHITESPACE);

    if (idx == string::npos || cmd_line[idx] != '&') {
        return false;
    }

    return true;
}

const char *Command::getCmdLine() const {
    return m_rawCmdLine;
}

BuiltInCommand::BuiltInCommand(const char *cmd_line):
    Command(cmd_line)
{}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmdLineStr = string(cmd_line);

    if(cmdLineStr.find('>') != string::npos) {
        return new RedirectionCommand(cmd_line);
    }

    if(cmdLineStr.find('|') != string::npos) {
      return new PipeCommand(cmd_line);
    }

    if(cmdLineStr.find("timeout") != string::npos) {
        return new TimeoutCommand(cmd_line);
    }

    return findCommand(cmd_line, false);
}

Command *SmallShell::findCommand(const char *cmd_line, bool isPipe, int timeout) {
    string cmd_s = _trim(_removeBackgroundSign(cmd_line));
    string commandStr = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if(commandStr == "timeout") {
        std::istringstream iss(_trim(string(cmd_line)).c_str());
        std::string s;
        for(int i=0; i<=2; i++) {
            iss >> s;
        }
        commandStr = s;
    }

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
        auto cdCom = new ChangeDirCommand(cmd_line, &lastPwd);
        return cdCom;
    }

   else if(commandStr == "setcore") {
        return new SetcoreCommand(cmd_line, SmallShell::getInstance().getJobsList());
    }

    else if(commandStr == "chmod") {
        return new ChmodCommand(cmd_line);
    }

    else if(commandStr == "fg") {
        return new ForegroundCommand(cmd_line, SmallShell::getInstance().getJobsList());
    }


    else if(commandStr == "jobs") {
        return new JobsCommand(cmd_line, SmallShell::getInstance().getJobsList());
    }

    else if(commandStr == "bg") {
        return new BackgroundCommand(cmd_line, SmallShell::getInstance().getJobsList());
    }

    else if(commandStr == "quit") {
        return new QuitCommand(cmd_line, SmallShell::getInstance().getJobsList());
    }

    else if(commandStr == "kill") {
        return new KillCommand(cmd_line, SmallShell::getInstance().getJobsList());
    }

    else if(commandStr == "getfiletype") {
        return new GetFileTypeCommand(cmd_line);
    }

    return new ExternalCommand(cmd_line, isPipe, timeout);
}

void SmallShell::executeCommand(const char *cmd_line) {
    try {
        jobs->removeFinishedJobs();
        Command* cmd = CreateCommand(cmd_line);
        cmd->execute();
        delete cmd;
    }
    catch (const SyscallException& error) {
        if(error.m_fromChild) {
            ::perror(error.what());
            exit(0);
        }
        else
            ::perror(error.what());
    } catch(const exception& error) {
        cerr << error.what() << endl;
    }
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}


ShowPidCommand::ShowPidCommand(const char *cmd_line):
        BuiltInCommand(cmd_line),
        m_pid(getpid())
{}

void ShowPidCommand::execute() {
    cout << "smash pid is " << m_pid << endl;
}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line) :
        BuiltInCommand(cmd_line)
{
    if(m_numArgs == 0) {
        char cStringnewPropmpt[] = "smash> ";
        m_newPrompt = cStringnewPropmpt;
    }
    else {
        m_newPrompt = m_args[1];
        m_newPrompt = m_newPrompt + "> ";
    }

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


ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string* plastPwd):
        BuiltInCommand(cmd_line)
{
    if(m_numArgs > MAX_ARGS) {
        throw TooManyArgsError();
    }

    char* arg = m_args[1];

    if(arg == string("-")) {
        if (*plastPwd == string("")) {
            throw NoPWDError();
        }
        m_targetPwd = *plastPwd;
    } else {
        m_targetPwd = arg;
    }
}

void ChangeDirCommand::execute() {
    string lastDir = SmallShell::getInstance().getCurrDir();
    int retValue = chdir(m_targetPwd.c_str());
    if(retValue == RET_VALUE_ERROR) {
        throw SyscallException("chdir");
    }

    SmallShell::getInstance().setLastDir(lastDir);
    SmallShell::getInstance().setCurrDir(getcwd(nullptr, 0));
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) :
    BuiltInCommand(cmd_line)
{ }

void JobsCommand::execute() {
    SmallShell::getInstance().getJobsList()->printJobsList();
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs):
        BuiltInCommand(cmd_line),
        m_jobs(jobs)
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

    m_jobs->removeJobById(m_job->getJobId(), false);
    SmallShell::getInstance().setForegroundJob(m_job);

    if(waitpid(m_job->getPid(), nullptr, WUNTRACED) == RET_VALUE_ERROR) {
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
    if(m_bgJob == nullptr) {
        return;
    }
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
    if(m_numArgs > NO_ARGS && m_args[1] == string("kill")) {
        execKill = true;
    }
}

void QuitCommand::execute() {
    if(execKill) {
        cout << "smash: sending SIGKILL signal to " << m_jobs->getNumJobs() << " jobs:" << endl;
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

    if(m_sig == SIGCONT) {
        m_killJob->continueJob();
    }

    if(kill(pid, m_sig) == RET_VALUE_ERROR) {
        throw SyscallException("kill");
    }
    cout << "signal number " << m_sig << " was sent to pid " << pid << endl;
}

/* ----------------------------------------------- External Commands ------------------------------------------------- */

ExternalCommand::ExternalCommand(const char *cmd_line, bool isPipe, int timeout):
        Command(cmd_line),
        isPipe(isPipe),
        m_timeoutSecs(timeout)
{
    string cmdlineStr = m_cmdLine;
    if(cmdlineStr.find('*') != string::npos || cmdlineStr.find('?') != string::npos) {
        isSimple = false;
    } else {
        isSimple = true;
    }
}

void ExternalCommand::execute() {
    string cmdlineStr = m_cmdLine;
    int forkPid = 0;
    if(!isSimple) {
        if(isPipe) {
            execComplexChild();
            return;
        }

        forkPid = fork();
        if(forkPid > 0) {
            if(!m_isBackground) {
                auto jobEntry = new JobsList::JobEntry(0, forkPid, m_cmdLine, false, m_timeoutSecs);
                SmallShell::getInstance().setForegroundJob(jobEntry);
                if(waitpid(forkPid, nullptr, 0) == RET_VALUE_ERROR)
                    throw SyscallException("waitpid");
                SmallShell::getInstance().setForegroundJob(nullptr);
            }
            else
                SmallShell::getInstance().getJobsList()->addJob(this->m_rawCmdLine, forkPid, 0, false, m_timeoutSecs);
        }

        else if(forkPid == 0) {
            setpgrp();
            execComplexChild();
        }

        else {
            throw SyscallException("fork");
        }
    }

    else {
        if(isPipe) {
            execSimpleChild();
            return;
        }
        execSimpleCommand();
    }

}

void ExternalCommand::execSimpleCommand() {
    pid_t pid = fork();
    // Child process
    if(pid == 0) {
        setpgrp();
        execSimpleChild();
    }
    // Parent process
    else if(pid > 0) {
        if(!m_isBackground) {
            auto jobEntry = new JobsList::JobEntry(0, pid, getCmdLine(), false, m_timeoutSecs);
            SmallShell::getInstance().setForegroundJob(jobEntry);

            if(waitpid(pid, nullptr, WUNTRACED) == RET_VALUE_ERROR ) {
                SmallShell::getInstance().removeForegroundJob();
                throw SyscallException("waitpid");
            }
            SmallShell::getInstance().removeForegroundJob();
        } else {
            SmallShell::getInstance().getJobsList()->addJob(getCmdLine(), pid, 0, false, m_timeoutSecs);
        }
    }

    else {
        throw SyscallException("fork");
    }
}

void ExternalCommand::execComplexChild() {
    setpgrp();
    char* bashArgs[] = {(char*)"bash",(char*)"-c", (char*)m_cmdLine, nullptr};
    if(execvp("bash", bashArgs) == RET_VALUE_ERROR)
        throw SyscallException("execvp", true);
}

void ExternalCommand::execSimpleChild() {
    if(execvp(m_args[0], m_args) == RET_VALUE_ERROR) {
        throw SyscallException("execvp", true);
    }
}



/* ----------------------------------------------- Special Commands ------------------------------------------------- */

RedirectionCommand::RedirectionCommand(const char *cmd_line):
    Command(cmd_line)
{
    string delimiter;
    if(string(cmd_line).find(">>") != string::npos) {
        delimiter = ">>";
        m_overrideContent = false;
    } else {
        delimiter = ">";
        m_overrideContent = true;
    }

    string commands[2];
    Command::splitCommand(string(cmd_line), delimiter, commands);
    m_cmd = SmallShell::findCommand(commands[0].c_str(), true);
    m_fileName = commands[1];
}

void RedirectionCommand::execute() {
    int pid = fork();
    if(pid == 0) {
        setpgrp();
    // Close stdout fd so that when we open a file it will point the file object
        if(close(STDOUT_FILENO) == RET_VALUE_ERROR) {
            throw SyscallException("close");
        }

        int openMode = O_RDWR | O_CREAT;
        if(!m_overrideContent) {
            openMode = openMode | O_APPEND;
        } else {
            openMode = openMode | O_TRUNC;
        }
        open(m_fileName.c_str(), openMode, 0666);
        m_cmd->execute();
        exit(0);
    } else if(pid < 0) {
        throw SyscallException("fork");
    }
    waitpid(pid, nullptr, 0);
}

PipeCommand::PipeCommand(const char *cmd_line):
    Command(cmd_line, true)
{
    string delimiter;
    if(string(cmd_line).find("|&") != string::npos) {
        delimiter = "|&";
        m_useStderr = true;
    } else {
        delimiter = "|";
        m_useStderr = false;
    }

    string commands[2];
    Command::splitCommand(string(cmd_line), delimiter, commands);
    m_src = SmallShell::findCommand(commands[0].c_str(), true);
    m_dest = SmallShell::findCommand(commands[1].c_str(), true);
}

void PipeCommand::execute() {
    int fd[2];
    pipe(fd);
    int pipeOut = 1;
    if(m_useStderr) {
        pipeOut = 2;
    }

    int srcPid = fork();
    if(srcPid == 0) {
        setpgrp();
        // Map stdout to pipe write
        dup2(fd[1], pipeOut);
        if(m_useStderr) {
            dup2(2, 1);
        }

        safeClose(fd[0], true);
        safeClose(fd[1], true);
        m_src->execute();
        exit(0); // We exit because builtin commands don't use execv and will return from execute
    } else if(srcPid < 0) {
        throw SyscallException("fork");
    }

    int destPid = fork();
    if(destPid == 0) {
        setpgrp();
        // Map stdin to pipe read
        dup2(fd[0], 0);
        safeClose(fd[0], true);
        safeClose(fd[1], true);
        m_dest->execute();
        exit(0);
    } else if(destPid < 0) {
        throw SyscallException("fork");
    }

    safeClose(fd[0]);
    safeClose(fd[1]);

    if(waitpid(srcPid, nullptr, 0) == RET_VALUE_ERROR) {
        throw SyscallException("waitpid");
    }

    if(waitpid(destPid, nullptr, 0) == RET_VALUE_ERROR) {
        throw SyscallException("waitpid");
    }


}

void PipeCommand::safeClose(int fd, bool isChild) {
    if(close(fd) < 0) {
        throw SyscallException("close", isChild);
    }
}

SetcoreCommand::SetcoreCommand(const char *cmd_line, JobsList* jobs) :
    BuiltInCommand(cmd_line)
{
    int requestedJobId = 0;
    int requestedCore = 0;

    try {
        if(m_numArgs == Command::NO_ARGS) {
            throw SetCoreInvalidArguments();
        }
        requestedJobId = stoi(m_args[1]);
        requestedCore = stoi(m_args[2]);
        m_setCoreJob = jobs->getJobById(requestedJobId);
        if(requestedCore >= get_nprocs() || requestedCore < 0) {
            throw SetCoreInvalidCoreError();
        }
        m_core = requestedCore;
    }

    catch (invalid_argument& invalidArgument) {
        throw SetCoreInvalidArguments();
    }

    catch (JobNotFoundError& jobNotFoundError) {
        throw SetCoreJobNotFoundError(requestedJobId);
    }
}

void SetcoreCommand::execute() {
    cpu_set_t jobCPUMask;
    CPU_ZERO(&jobCPUMask);
    CPU_SET(m_core, &jobCPUMask);
    if(sched_setaffinity(m_setCoreJob->getPid(), sizeof(jobCPUMask), &jobCPUMask) == RET_VALUE_ERROR) {
        throw SyscallException("sched_setaffinity");
    }
}


ChmodCommand::ChmodCommand(const char *cmd_line) :
    BuiltInCommand(cmd_line)
{
    int requestedMode = 0;
    try {
        if (m_numArgs != CHMOD_NUM_ARGS)
            throw ChmodInvalidArguments();
        requestedMode = stoi(m_args[1],0, 8);
        m_path = m_args[2];
        m_mode = requestedMode;
    }
    catch (invalid_argument& invalidArgument) {
        throw ChmodInvalidArguments();
    }
}

void ChmodCommand::execute() {
    if(chmod(m_path, m_mode) == RET_VALUE_ERROR) {
        throw SyscallException("chmod");
    }
}


GetFileTypeCommand::GetFileTypeCommand(const char *cmd_line):
    BuiltInCommand(cmd_line)
{
    if(m_numArgs != NUM_ARGS) {
        throw FileTypeInvalidArgs();
    }

    fileName = m_args[1];
}

void GetFileTypeCommand::execute() {
    auto statBuffer = (struct stat*)::malloc(sizeof(struct stat));
    int retValue = lstat(fileName.c_str(), statBuffer);
    if(retValue) {
        throw SyscallException("lstat");
    }

    string fileType;
    switch(statBuffer->st_mode & S_IFMT) {
        case S_IFBLK:
            fileType = "block device";
            break;
        case S_IFCHR:
            fileType = "character device";
            break;
        case S_IFDIR:
            fileType = "directory";
            break;
        case S_IFIFO:
            fileType =  "FIFO";
            break;
        case S_IFLNK:
            fileType = "symbolic link";
            break;
        case S_IFREG:
            fileType = "regular file";
            break;
        case S_IFSOCK:
            fileType = "socket";
            break;
        default:
            fileType = "";
    }

    cout << fileName << "'s type is \"" + fileType + "\" and takes up " + to_string(statBuffer->st_size) + " bytes" << endl;
    free(statBuffer);
}


// Timeout with redirection?
TimeoutCommand::TimeoutCommand(const char *cmd_line):
    BuiltInCommand(cmd_line)
{
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    std::string s;
    iss >> s;
    iss >> s;

    stringstream secsStr;
    int secs;
    secsStr << s;
    secsStr >> secs;
    m_secs = secs;
    m_cmd = SmallShell::findCommand(cmd_line, false, m_secs);
}

TimeoutCommand::~TimeoutCommand() {
    delete m_cmd;
}

void TimeoutCommand::execute() {
    SmallShell::getInstance().addTimeout(m_secs);
    m_cmd->execute();
}

/* --------------------------------------------------- JOBS LIST ---------------------------------------------------- */
JobsList::~JobsList() {
    for(auto job: jobs) {
        delete job.second;
    }
}

void JobsList::addJob(const char* rawCmdLine, pid_t pid, int jobId, bool isStopped, int timeout) {
    removeFinishedJobs();
    if(jobId == 0) {
        jobId = assignJobId(jobs);
    }

    auto jobEntry = new JobEntry(jobId, pid, rawCmdLine, isStopped, timeout);
    jobs.insert({jobId, jobEntry});
}

int JobsList::assignJobId(map<int, JobEntry*> jobs) {
    if(jobs.empty()) {
        return 1;
    }

    return jobs.rbegin()->first + 1;
}

 void JobsList::removeJobById(int jobId, bool shouldDelete) {
    JobEntry* jobEntry = getJobById(jobId);
    jobs.erase(jobId);
    if(shouldDelete) {
        delete jobEntry;
    }
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

void JobsList::terminateTimedOutJobs() {
    for(auto it = jobs.begin(); it->first != jobs.end()->first;) {
        if(it->second->isTimedOut()) {
            JobEntry* jobEntry = it->second;
            if(kill(jobEntry->getPid(), SIGKILL) == RET_VALUE_ERROR) {
                throw SyscallException("kill");
            }
            cout << "smash: " + jobEntry->getCmdLine() + " timed out!" << endl;
            it = jobs.erase(it);
            delete jobEntry;
        } else {
            it++;
        }
    }
}

JobsList::JobEntry::JobEntry(int jobId, int jobPid, const char* cmdLine, bool isStopped, int timeoutSecs) :
    m_jobId(jobId),
    m_pid(jobPid),
    m_cmdLine(cmdLine),
    m_isStopped(isStopped),
    m_insertTime(time(nullptr)),
    m_timeoutSecs(timeoutSecs)
{ }

bool JobsList::JobEntry::isJobStopped() const {
    return m_isStopped;
}

int JobsList::JobEntry::getPid() const {
    return m_pid;
}

int JobsList::JobEntry::getJobId() const {
    return m_jobId;
}

string JobsList::JobEntry::getCmdLine() const {
    return m_cmdLine;
}

bool JobsList::JobEntry::isTimedOut() const {
    if(m_timeoutSecs != -1 && getTimeDiff() >= m_timeoutSecs) {
        return true;
    }

    return false;
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
        timeDiff = difftime(currTime, m_insertTime);
        timeStr = " " + to_string(int(timeDiff)) + " secs";
    }

    string stoppedFlag;
    if(m_isStopped && showStoppedFlag) {
        stoppedFlag = " (stopped)";
    }

    cout << "[" << m_jobId << "] " << m_cmdLine << " : " << m_pid << timeStr << stoppedFlag << endl;
}

void JobsList::JobEntry::printCmdLine() const {
    cout << m_cmdLine << " : " << m_pid << endl;
}

void JobsList::JobEntry::continueJob() {
    m_isStopped = false;
}

void JobsList::JobEntry::stopJob() {
    m_isStopped = true;
}

int JobsList::JobEntry::getTimeout() const {
    return m_timeoutSecs;
}

int JobsList::JobEntry::getTimeDiff() const {
    time_t currTime = time(nullptr);
    return difftime(currTime, m_insertTime);
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
    if(jobs.empty()) {
        return;
    }

    for (auto it = jobs.begin(); it != jobs.end();) {
        waitpidCheck = waitpid(it->second->getPid(), &status, WNOHANG);
        if(waitpidCheck > 0) {
            JobEntry* jobEntry = getJobById(it->first);
            it = jobs.erase(it);
            delete jobEntry;
        }
        else if(waitpidCheck == RET_VALUE_ERROR) {
            throw SyscallException("waitpid");
        } else {
            it++;
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
    if(jobs.empty()) {
        return nullptr;
    }
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
    for (auto job: jobs) {
        cout << job.second->getPid() << ": " << job.second->getCmdLine() << endl;
        kill(job.second->getPid(), SIGKILL);
    }
}

::size_t JobsList::getNumJobs() const {
    return jobs.size();
}

Timeout::Timeout(int id, int timeoutSecs):
    m_id(id),
    m_insertTime(time(nullptr)),
    m_timeoutSecs(timeoutSecs)
{}

int Timeout::getDiff() const {
    time_t currTime = time(nullptr);
    return difftime(m_insertTime+m_timeoutSecs, currTime);
}

int Timeout::getId() const {
    return m_id;
}



