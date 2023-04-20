#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <exception>
#include "Commands.h"
#include "Exception.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#define MAX_NUM_ARGS 20
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
    char tempPwd[80];
    m_currPwd = getcwd(tempPwd, 80);
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

Command::Command(const char *cmd_line):
    m_cmdLine(cmd_line)
{
    char* args[MAX_NUM_ARGS+1];
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

  else if(commandStr.compare("cd")) {

      char** plastPwd = nullptr;
      char* lastPwdCopy = nullptr;

      if(!m_lastPwdList.empty()) {
          plastPwd = m_lastPwdList.top();
          lastPwdCopy = *(m_lastPwdList.top());
      }

      auto cdCom =  new ChangeDirCommand(cmd_line, plastPwd);

      // If plastPwd wasn't changed, it means that we used "-" argument
      if(*plastPwd == lastPwdCopy) {
          m_lastPwdList.pop();
      } else {
          m_lastPwdList.push(plastPwd);
      }

      return cdCom;
  }

 return new ExternalCommand(cmd_line);

}

void ShowPidCommand::execute() {
    int pid = getpid();
    cout << "smash pid is " << pid << endl;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here

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


ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd):
        BuiltInCommand(cmd_line),
        m_plastPwd(plastPwd)
{
    if(m_numArgs > 1) {
        // TODO: throw exception and use define for 1
        return;
    }

    // TODO: check for invalid literals
    // TODO: check for the same path
    // TODO: more exceptions?


    char* arg = m_args[1];
    if(arg == "-" && plastPwd == nullptr) {
        // TODO: throw exception
        return;
    }

    *(this->m_plastPwd) = arg;
    *plastPwd = arg;
}

void ChangeDirCommand::execute() {
    int retValue = chdir(*m_plastPwd);
    if(retValue == RET_VALUE_ERROR) {
        throw SyscallChdirError();
    }
}