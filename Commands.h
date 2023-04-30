#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>
#include <list>
#include <stack>
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


using namespace std;

class Command {
public:
  explicit Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  const char* getCmdLine() const;

protected:
    const char* m_cmdLine;
    char** m_args;
    int m_numArgs;

    static const int NO_ARGS = 1;
    static const int CMD_MAX_NUM_ARGS = 20;
};

class BuiltInCommand: public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand: public Command {
 public:
  explicit ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  explicit PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand: public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangePromptCommand: public BuiltInCommand {
public:
    explicit ChangePromptCommand(const char* cmd_line);
    ~ChangePromptCommand() override = default;
    void execute() override;
private:
    char* m_newPrompt;
};

class ChangeDirCommand: public BuiltInCommand {
// TODO: Add your data members
public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;

private:
    static const int MAX_ARGS = 1;
    char* m_lastPwd;
};

class GetCurrDirCommand: public BuiltInCommand {
 public:
  explicit GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand: public BuiltInCommand {
 public:
  explicit ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobsList {
 public:
  class JobEntry {
  public:
      JobEntry(int jobId, int jobPid, Command *cmd, bool isStopped);
      bool isJobStopped() const;
      pid_t getPid() const;
      void print(bool includeTime = true) const;
      void printCmdLine() const;
      void continueJob();
      void stopJob();

  private:
      int m_jobId;
      pid_t m_pid;
      Command* m_cmd;
      bool m_isStopped;
      time_t m_insertTime;

      // TODO: pointer to last stopped job?
  };
 public:
    // TODO: Delete job entries
  ~JobsList();
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  // TODO: remember to delete* jobs!
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob();
  int getMaxJobId() const;
//  JobEntry* getJobById(char* jobId) const;
  // TODO: Add extra methods or modify exisitng ones as needed

private:
    static int assignJobId(map<int, JobEntry*> jobs);
    map<int, JobEntry*> jobs;
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  ~JobsCommand() override = default;
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
private:
    JobsList::JobEntry* m_job;
    static const int FG_MAX_NUM_ARGS = 1;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
private:
    JobsList::JobEntry* m_bgJob;
    static const int BG_MAX_NUM_ARGS = 1;


};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  GetFileTypeCommand(const char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
private:
    JobsList::JobEntry* m_killJob;
    int m_sig;
};

class SmallShell {
private:
    SmallShell();
    stack<char*> m_lastPwdList;
    char* m_currPwd;
    std::string m_smashPrompt;
    JobsList* jobs;

    static Command* handleChdirCommand(stack<char*>* lastPwdList, char** currPwd, const char* cmd_line);

public:
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator

    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
        return instance;
    }

    ~SmallShell();
    void executeCommand(const char* cmd_line);
    const std::string& getPrompt() const;
    void setPrompt(const std::string& new_prompt);
    const char* getCurrDir() const;
    JobsList* getJobsList() const;

  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
