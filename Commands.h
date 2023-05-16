#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <map>
#include <list>
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


using namespace std;

class Command {
public:
    explicit Command(const char* cmd_line, bool skipBgRemove = false);
    virtual ~Command();
    virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
    const char* getCmdLine() const;

    static void splitCommand(const string& str, const string& delimiter, string commands[2]);

    static const int MAX_COMMAND_SIZE = 80;
    static const int CMD_MAX_NUM_ARGS = 20;

protected:
    static const int NO_ARGS = 0;

    const char* m_cmdLine;
    const char* m_rawCmdLine;
    char* m_args[CMD_MAX_NUM_ARGS+1];
    int m_numArgs;
    bool m_isBackground;

private:
    static bool hasBackgroundSign(string cmd_line);

};

class BuiltInCommand: public Command {
public:
    explicit BuiltInCommand(const char* cmd_line);
    ~BuiltInCommand() override = default;
};

class ExternalCommand: public Command {
 public:
  explicit ExternalCommand(const char* cmd_line, bool isPipe);
  virtual ~ExternalCommand() = default;
  void execute() override;

private:
    bool isSimple;
    bool isPipe;

    void execSimpleCommand();
    void execComplexChild();
    void execSimpleChild();
};


class PipeCommand : public Command {
 public:
  explicit PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
private:
    static void safeClose(int fd, bool isChild = false);

    Command* m_src;
    Command* m_dest;
    bool m_useStderr;
};

class RedirectionCommand: public Command {
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;

private:
    bool m_overrideContent;
    Command* m_cmd;
    string m_fileName;
};

class ChangePromptCommand: public BuiltInCommand {
public:
    explicit ChangePromptCommand(const char* cmd_line);
    ~ChangePromptCommand() override = default;
    void execute() override;
private:
    string m_newPrompt;
};

class ChangeDirCommand: public BuiltInCommand {
// TODO: Add your data members
public:
    ChangeDirCommand(const char *cmd_line, string* plastPwd);
    ~ChangeDirCommand() override = default;
    void execute() override;

private:
    static const int MAX_ARGS = 1;
    string m_targetPwd;
};

class GetCurrDirCommand: public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char* cmd_line);
    ~GetCurrDirCommand() override = default;
    void execute() override;
};

class ShowPidCommand: public BuiltInCommand {
public:
    explicit ShowPidCommand(const char* cmd_line);
    ~ShowPidCommand() override = default;
    void execute() override;
private:
    pid_t m_pid;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    ~QuitCommand() override = default;
    void execute() override;

private:
    bool execKill;
    JobsList* m_jobs;
};

class JobsList {
public:
    class JobEntry {
    public:
        JobEntry(int jobId, int jobPid, const char* cmdLine, bool isStopped);
        bool isJobStopped() const;
        pid_t getPid() const;
        int getJobId() const;
        string getCmdLine() const;
        void print(bool showStoppedFlag, bool includeTime = true) const;
        void printCmdLine() const;
        void continueJob();
        void stopJob();

    private:
        int m_jobId;
        pid_t m_pid;
        string m_cmdLine;
        bool m_isStopped;
        time_t m_insertTime;

      // TODO: pointer to last stopped job?
  };
 public:
    JobsList() = default;
    ~JobsList();
    void addJob(const char* rawCmdLine, pid_t pid, int jobId, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId, bool shouldDelete = true);
//  JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob();
    int getMaxJobId() const;
    bool isEmpty() const;
    ::size_t getNumJobs() const;
//  JobEntry* getJobById(char* jobId) const;

private:
    map<int, JobEntry*> jobs;
    static int assignJobId(map<int, JobEntry*> jobs);
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
    ~ForegroundCommand() override = default;
    void execute() override;
private:
    JobsList::JobEntry* m_job;
    JobsList* m_jobs;
    static const int FG_MAX_NUM_ARGS = 1;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    ~BackgroundCommand() override = default;
    void execute() override;
private:
    JobsList::JobEntry* m_bgJob;
    static const int BG_MAX_NUM_ARGS = 1;
};


class ChmodCommand : public BuiltInCommand {
public:
    explicit ChmodCommand(const char* cmd_line);
    ~ChmodCommand() override = default;
    void execute() override;
private:
    mode_t m_mode;
    const char* m_path;
    const int CHMOD_NUM_ARGS = 2;

};

//class TimeoutCommand : public BuiltInCommand {
//public:
//    explicit TimeoutCommand(const char* cmd_line);
//    virtual ~TimeoutCommand() {}
//    void execute() override;
//private:
//    Command* m_cmd;
//    unsigned int m_secs;
//};

class GetFileTypeCommand : public BuiltInCommand {
 public:
  GetFileTypeCommand(const char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
private:
    string fileName;
    static const int NUM_ARGS = 1;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
    SetcoreCommand(const char* cmd_line, JobsList* jobs);
    ~SetcoreCommand() override = default;
    void execute() override;
private:
    JobsList::JobEntry* m_setCoreJob;
    int m_core;
};

class KillCommand : public BuiltInCommand {
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    ~KillCommand() override = default;
    void execute() override;
private:
    JobsList::JobEntry* m_killJob;
    int m_sig;
    static const int KILL_NUM_ARGS = 2;
    static const char KILL_SIGNAL_PREFIX = '-';
};

class SmallShell {
private:
    SmallShell();
    std::string m_smashPrompt;
    JobsList* jobs;
    JobsList::JobEntry* m_foregroundJob;
    string m_lastPwd;
    string m_currPwd;

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
    string getCurrDir() const;
    string getLastDir() const;
    void setCurrDir(const string& dir);
    void setLastDir(const string& dir);
    JobsList* getJobsList() const;
    JobsList::JobEntry* getForegroundJob() const;
    void setForegroundJob(JobsList::JobEntry* jobEntry);
    void removeForegroundJob();

    static Command* findCommand(const char* cmd_line, bool isPipe = false);

    static const int RET_VALUE_ERROR = -1;
};

#endif //SMASH_COMMAND_H_
