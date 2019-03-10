#ifndef GDBE_H_
#define GDBE_H_

#include "syntax_tree.h"
#include "result.h"
#include "error.h"
#include "query_optimizer.h"
#include "buffer_pool.h"
#include "file_system.h"
#include "database_schema.h"

const std::string kDatabaseDir = "database/";

class GDBE
{
public:
  GDBE():buffer_pool_(BufferPool::getInstance()),file_system_(FileSystem::getInstance()){}
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
  void execShowTables(const Node &);
  void execShowIndex(const Node &){};
  void execDropDatabase(const Node &);
  void execDropTable(const Node &){};
  void execDropIndex(const Node &){};
  std::string getTableName(const Node &name_node)
  {
    switch (name_node.childern.size())
    {
    case 1:
      return name_node.childern.front().token.str;
    case 2:
    case 3:
      if (name_node.childern.front().token.str != database_name_)
        throw Error(kIncorrectDatabaseNameError, name_node.childern.front().token.str);
      return name_node.childern[1].token.str;
    default:
      throw Error(kIncorrectNameError, name_node.token.str);
    }
  }
  std::string getColumnName(const Node &name_node, const std::string &table_name = "")
  {
    switch (name_node.childern.size())
    {
    case 1:
      return name_node.childern.front().token.str;
    case 3:
      if (name_node.childern.front().token.str != database_name_)
        throw Error(kIncorrectDatabaseNameError, name_node.childern.front().token.str);
      if (!table_name.empty() && name_node.childern[1].token.str != table_name)
        throw Error(kIncorrectTableNameError, name_node.childern[1].token.str);
      return name_node.childern.front().token.str;
    case 2:
      if (!table_name.empty() && name_node.childern.front().token.str != table_name)
        throw Error(kIncorrectTableNameError, name_node.childern.front().token.str);
      return name_node.childern.back().token.str;
    default:
      throw Error(kIncorrectNameError, name_node.token.str);
    }
  }
  std::string getBothTableName(const Node &name_node)
  {
    switch (name_node.childern.size())
    {
    case 2:
      return name_node.childern.front().token.str;
    case 3:
      if (name_node.childern.front().token.str != database_name_)
        throw Error(kIncorrectDatabaseNameError, name_node.childern.front().token.str);
      return name_node.childern[1].token.str;
    default:
      throw Error(kIncorrectNameError, name_node.token.str);
    }
  }

  size_t getFreePage(){
    if(database_schema_.free_page_deque.empty())
      return ++database_schema_.max_page;
    else
    {
      size_t temp=database_schema_.free_page_deque.front();
      database_schema_.free_page_deque.pop_front();
      return temp;
    }
  };

private:
  QueryOptimizer query_optimizer_;
  BufferPool& buffer_pool_;
  FileSystem& file_system_;
  SyntaxTree syntax_tree_;
  Result result_;
  std::string database_name_;
  DatabaseSchema database_schema_;
};

#endif
