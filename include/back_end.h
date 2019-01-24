#ifndef BACKEND_H_
#define BACKEND_H_

#include "syntax_tree.h"
#include "query_optimizer.h"
#include "gdbe.h"
#include "buffer_pool.h"

class BackEnd{
    public:
        BackEnd()=default;
        ~BackEnd()=default;
        BackEnd(const BackEnd&)=delete;
        BackEnd(BackEnd&&)=delete;
        BackEnd& operator=(BackEnd)=delete;
        int Start(SyntaxTree&&);
    private:
        QueryOptimizer query_optimizer_;
        GDBE gdbe_;
        BufferPool buffer_pool_;
        SyntaxTree& syntax_tree_;
        
};

#endif
