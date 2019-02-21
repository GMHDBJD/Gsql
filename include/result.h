#ifndef RESULT_H_
#define RESULT_H_

#include "syntax_tree.h"

class Result{
    public:
        Result()=default;
        ~Result()=default;
        Result(const Result&);
        Result(Result&&);
        Result& operator=(Result);
        int clear();
        operator bool();
    private:
};

#endif