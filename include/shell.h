#ifndef SHELL_H_
#define SHELL_H_

#include <string>
#include "result.h"
#include "error.h"
#include <readline/readline.h>
#include <readline/history.h>

static int doNothing(int count, int key)
{
  return 0;
}

class Shell
{
public:
  Shell()
  {
    rl_bind_key('\t', doNothing);
  }
  ~Shell() = default;
  Shell(const Shell &) = delete;
  Shell(Shell &&) = delete;
  Shell &operator=(Shell) = delete;
  std::string getInput();
  void showResult(const Result &);
  void showError(const Error &error);

private:
  std::string buffer;
};

#endif
