#ifndef GDBE_H_
#define GDBE_H_

#include "syntax_tree.h"
#include "result.h"
#include "error.h"
#include "query_optimizer.h"
#include "buffer_pool.h"
#include "filesystem.h"
#include "database_schema.h"

const std::string kDatabaseDir = "database/";

class GDBE
{
public:
  GDBE() = default;
  ~GDBE() = default;
  GDBE(const GDBE &) = delete;
  GDBE(GDBE &&) = delete;
  GDBE &operator=(GDBE) = delete;
  void exec(SyntaxTree);
  Result getResult();
  void execRoot(const Node &);
  void execCreate(const Node &);
  void execShow(const Node &);
  void execUse(const Node &);
  void execInsert(const Node &){};
  void execSelect(const Node &){};
  void execUpdate(const Node &){};
  void execDelete(const Node &){};
  void execAlter(const Node &){};
  void execDrop(const Node &);
  void execCreateTable(const Node &);
  void execCreateDatabase(const Node &);
  void execCreateIndex(const Node &){};
  void execShowDatabases(const Node &);
  void execShowTables(const Node &){};
  void execShowIndex(const Node &){};
  void execDropDatabase(const Node &);
  void execDropTable(const Node &){};
  void execDropIndex(const Node &){};

  size_t getFreePage(){};

private:
  QueryOptimizer query_optimizer_;
  BufferPool buffer_pool_;
  SyntaxTree syntax_tree_;
  Result result_;
  std::string database_name_;
  DatabaseSchema database_schema_;
};

#endif
