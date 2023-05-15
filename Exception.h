#ifndef SKELETON_SMASH_EXCEPTION_H
#define SKELETON_SMASH_EXCEPTION_H

#include <exception>
#include <cstring>

using namespace std;

class TooManyArgsError: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: cd: too many arguments";
    }
};

class NoPWDError: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: cd: OLDPWD not set";
    }
};

class JobNotFoundError: public exception {};

class FgJobNotFoundError: public exception {
public:
    explicit FgJobNotFoundError(int jobId):
            m_errorStr("smash error: fg: job-id " + std::to_string(jobId) + " does not exist")
//        jobId(jobId)
    {}

    const char* what() const noexcept override {
        // TODO: fix this error message
        return m_errorStr.c_str();
//        return "smash error: fg: job-id <job-id> does not exist";
    }

private:
    std::string m_errorStr;
};

class JobsListIsEmptyError: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: fg: jobs list is empty";
    }
};

class FgInvalidArgumentsError: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: fg: invalid arguments";
    }
};

class BgInvalidArgumentsError: public  exception {
public:
    const char* what() const noexcept override {
        return "smash error: bg: invalid arguments";
    }
};

class BgJobNotFoundError : public exception {
public:
    explicit BgJobNotFoundError(int jobId):
        m_errorStr("smash error: bg: job-id " + std::to_string(jobId) + " does not exist")
    {}

    const char* what() const noexcept override {
        return m_errorStr.c_str();
    }
private:
    std::string m_errorStr;
};

class BgJobNotStoppedError : public exception {
public:
    explicit BgJobNotStoppedError(int jobId):
        m_errorStr("smash error: bg: job-id " + std::to_string(jobId) + " is already running in the background")
    {}

    const char* what() const noexcept override {
        return m_errorStr.c_str();
    }
private:
    std::string m_errorStr;
};

class BgNoStoppedJobs : public  exception {
public:
    const char* what() const noexcept override {
        return "smash error: bg: there is no stopped jobs to resume";
    }
};

class NoStoppedJobs {};

class KillInvalidArgumentsError : public exception {
public:
    const char* what() const noexcept override {
        return "smash error: kill: invalid arguments";
    }
};

class KillJobNotFoundError : public exception {
public:
    explicit KillJobNotFoundError(int jobId) :
        m_errorStr("smash error: kill: job-id " + std::to_string(jobId) + " does not exist")
    {}

    const char* what() const noexcept override {
        return m_errorStr.c_str();
    }

private:
    std::string m_errorStr;
};

class FileTypeInvalidArgs: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: gettype: invalid arguments";
    }
};

// SyscallException
class SyscallException: public exception {
public:
    explicit SyscallException(const char* syscallName, bool fromChild = false) :
        m_fromChild(fromChild),
        m_syscallName(syscallName),
        m_errorStr("smash error: " + m_syscallName + " failed")
    {}
    const char* what() const noexcept override {
        return m_errorStr.c_str();
    }
    bool m_fromChild;
private:
    std::string m_syscallName;
    std::string m_errorStr;
};

class SetCoreJobNotFoundError: public exception {
public:
    explicit SetCoreJobNotFoundError(int jobid) :
        m_errorStr("smash error: setcore: job-id " + to_string(jobid) + " does not exist")
    {}
    const char* what() const noexcept override {
        return m_errorStr.c_str();
    }

private:
    string m_errorStr;
};

class SetCoreInvalidCoreError: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: setcore: invalid core number";
    }
};

class SetCoreInvalidArguments: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: setcore: invalid arguments";
    }
};

class ChmodInvalidArguments: public exception {
public:
    const char* what() const noexcept override {
        return "smash error: chmod: invalid arguments";
    }
};





#endif //SKELETON_SMASH_EXCEPTION_H
