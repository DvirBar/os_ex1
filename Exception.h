#ifndef SKELETON_SMASH_EXCEPTION_H
#define SKELETON_SMASH_EXCEPTION_H

#include <exception>
#include <string.h>

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

// SyscallException
class SyscallException: public exception {};

class SyscallChdirError: public SyscallException {
public:
    const char* what() const noexcept override {
        return "smash error: chdir failed";
    }
};




#endif //SKELETON_SMASH_EXCEPTION_H
