#ifndef GDBE_H_
#define GDBE_H_

#include "syntax_tree.h"

class GDBE{
    public:
        GDBE()=default;
        ~GDBE()=default;
        GDBE(const GDBE&)=delete;
        GDBE(GDBE&&)=delete;
        GDBE& operator=(GDBE)=delete;
        void exec(const SyntaxTree&);
    private:
};

#endif
