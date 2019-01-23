#ifndef SHELL_H_
#define SHELL_H_

#include <string>

class Shell{
    public:
        Shell()=default;
        ~Shell()=default;
        Shell(const Shell&)=delete;
        Shell(Shell&&)=delete;
        Shell& operator=(Shell)=delete;
        int GetInput(std::string*);
};

#endif
