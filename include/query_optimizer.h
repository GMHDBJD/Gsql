#ifndef QUERY_OPTIMIZER_H_
#define QUERY_OPTIMIZER_H_
#include "syntax_tree.h"

class QueryOptimizer{
    public:
        QueryOptimizer()=default;
        ~QueryOptimizer()=default;
        QueryOptimizer(const QueryOptimizer&)=delete;
        QueryOptimizer(QueryOptimizer&&)=delete;
        QueryOptimizer& operator=(QueryOptimizer)=delete;
        int optimizer(SyntaxTree*);
    private:
};

#endif
