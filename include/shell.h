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
    int getInput(std::string *);
    int showResult(const Result &);
};

#endif
