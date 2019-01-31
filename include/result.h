#ifndef RESULT_H_
#define RESULT_H_

#include "syntax_tree.h"

class Result{
    public:
        Result()=default;
        ~Result()=default;
        Result(const Result&)=delete;
        Result(Result&&)=delete;
        Result& operator=(Result)=delete;
        int clear();
    private:
};

#endif