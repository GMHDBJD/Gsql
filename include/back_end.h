#ifndef BACKEND_H_
#define BACKEND_H_

#include "syntax_tree.h"

class BackEnd{
    public:
        BackEnd()=default;
        ~BackEnd()=default;
        BackEnd(const BackEnd&)=delete;
        BackEnd(BackEnd&&)=delete;
        BackEnd& operator=(BackEnd)=delete;
        int exec(SyntaxTree syntax_tree);
};

#endif
