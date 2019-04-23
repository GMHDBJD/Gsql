#ifndef GDBE_H_
#define GDBE_H_

#include "syntax_tree.h"
#include "result.h"
#include "error.h"
#include "query_optimizer.h"
#include "buffer_pool.h"
#include "file_system.h"
#include "database_schema.h"
#include "stream.h"
#include "utility.h"

const std::string kDatabaseDir = "database/";

class GDBE
{
public:
  ~GDBE() = default;
  static GDBE &getInstance();
  GDBE(const GDBE &) = delete;
  GDBE(GDBE &&) = delete;
  GDBE &operator=(GDBE) = delete;
  void exec(SyntaxTree);
  Result getResult();
  void execRoot(Node &);
  void execCreate(const Node &);
  void execShow(const Node &);
  void execUse(const Node &);
  void execInsert(Node &);
  void execSelect(Node &);
  void execUpdate(const Node &);
  void execDelete(Node &);
  void execAlter(const Node &);
  void execDrop(const Node &);
  void execCreateTable(const Node &);
  void execCreateDatabase(const Node &);
  void execCreateIndex(const Node &);
  void execShowDatabases(const Node &);
  void execShowTables(const Node &);
  void execShowIndex(const Node &);
  void execDropDatabase(const Node &);
  void execDropTable(const Node &);
  void execDropIndex(const Node &);
  void execExplain(const Node &);
  size_t getExprDataType(const Node &node);
  size_t getValueSize(const std::unordered_map<std::string, ColumnSchema> &column_schema_map);
  void updateDatabaseSchema();
  void selectRecursive(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &, const std::unordered_set<std::string> &, const std::vector<Node> &select_expr_vector, size_t limit);
  void selectRecursiveAux(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, std::unordered_set<std::string> table_set, std::unordered_set<std::string>, const std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column, const std::vector<Node> &select_expr_vector, size_t limit);
  void deleteRecursive(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &, const std::unordered_set<std::string> &, const std::unordered_set<std::string> &, std::unordered_map<std::string, size_t> &table_id_page_map);
  void deleteRecursiveAux(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, std::unordered_set<std::string> table_set, std::unordered_set<std::string>, const std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column, const std::unordered_set<std::string> &, std::unordered_map<std::string, size_t> &table_id_page_map, std::unordered_map<std::string, size_t> table_id_map);
  std::pair<IndexSchema, std::pair<Token, Token>> getCondition(std::vector<Node> &expr_vector, bool *rc);
  bool isIndexCondition(Node &node);

  size_t getFreePage()
  {
    if (database_schema_.free_page_deque.empty())
      return ++database_schema_.max_page;
    else
    {
      size_t temp = database_schema_.free_page_deque.front();
      database_schema_.free_page_deque.pop_front();
      return temp;
    }
  }

  void addFreePage(size_t page_id)
  {
    database_schema_.free_page_deque.push_back(page_id);
  }

private:
  GDBE() : buffer_pool_(BufferPool::getInstance()), file_system_(FileSystem::getInstance()) {}
  QueryOptimizer query_optimizer_;
  BufferPool &buffer_pool_;
  FileSystem &file_system_;
  SyntaxTree syntax_tree_;
  Result result_;
  std::string database_name_;
  DatabaseSchema database_schema_;
};

#endif
