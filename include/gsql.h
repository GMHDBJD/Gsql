#ifndef GSQL_H_
#define GSQL_H_

#include "query_optimizer.h"
#include "gdbe.h"
#include "buffer_pool.h"
#include "syntax_tree.h"
#include "result.h"

class Gsql
{
  public:
    Gsql() = default;
    Gsql(const Gsql &) = delete;
    Gsql(Gsql &&) noexcept = delete;
    Gsql &operator=(Gsql) = delete;
    ~Gsql() = default;
    int run(SyntaxTree &&);
    int getResult(Result*);

  private:
    QueryOptimizer query_optimizer_;
    GDBE gdbe_;
    BufferPool buffer_pool_;
};

#endif
