#ifndef GSQL_H_
#define GSQL_H_

#include "shell.h"
#include "front_end.h"
#include "back_end.h"

class Gsql
{
    public:
        Gsql()=default;
        Gsql(const Gsql&)=delete;
        Gsql(Gsql&&) noexcept =delete;
        Gsql& operator=(Gsql)=delete;
        ~Gsql()=default;
        void run();
    private:
        Shell shell;
        FrontEnd front_end;
        BackEnd back_end;
};

#endif
