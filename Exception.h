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
        jobId(jobId)
    {}

    const char* what() const noexcept override {
        // TODO: fix this error message
        return "smash error: fg: job-id <job-id> does not exist";
    }

private:
    int jobId;
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

// SyscallException
class SyscallException: public exception {};

class SyscallChdirError: public SyscallException {
public:
    const char* what() const noexcept override {
        return "smash error: cd: chdir failed";
    }
};




#endif //SKELETON_SMASH_EXCEPTION_H
