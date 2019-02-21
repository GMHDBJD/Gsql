#ifndef SHELL_H_
#define SHELL_H_

#include <string>
#include "result.h"

class Shell
{
  public:
    Shell() = default;
    ~Shell() = default;
    Shell(const Shell &) = delete;
    Shell(Shell &&) = delete;
    Shell &operator=(Shell) = delete;
    std::string getInput();
    void showResult(const Result &);
    void showError(const Error&);
};

#endif
