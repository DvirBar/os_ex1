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

class JobNotFoundError: public exception {
public:
    explicit JobNotFoundError(int jobId):
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
    const char* what() const noexcept override {
        return "smash error: bg: there is no stopped jobs to resume";
    }
};

class NoStoppedJobs {};

class KillInvalidArgumentsError : public exception {
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

// SyscallException
class SyscallException: public exception {
public:
    explicit SyscallException(const char* syscallName) :
        m_syscallName(syscallName),
        m_errorStr("smash error: " + m_syscallName + " failed")
    {}
    const char* what() const noexcept override {
        return m_errorStr.c_str();
    }
private:
    std::string m_syscallName;
    std::string m_errorStr;
};

//class SyscallChdirError: public SyscallException {
//public:
//    const char* what() const noexcept override {
//        return "smash error: cd: chdir failed";
//    }
//};
//




#endif //SKELETON_SMASH_EXCEPTION_H
