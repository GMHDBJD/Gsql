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
  void execUpdate(const Node &){};
  void execDelete(Node &);
  void execAlter(const Node &){};
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
  size_t getExprDataType(const Node &node)
  {
    if (node.token.token_type == kName)
    {
      int data_type = database_schema_.table_schema_map[node.children.front().token.str].column_schema_map[node.children.back().token.str].data_type;
      return data_type;
    }
    else
    {
      return 0;
    }
  }

  size_t getValueSize(const std::unordered_map<std::string, ColumnSchema> &column_schema_map)
  {
    size_t size = 0;
    size += (column_schema_map.size() + 1) * kSizeOfBool;
    size += kSizeOfSizeT;
    for (auto &&i : column_schema_map)
    {
      if (i.second.data_type == 0)
        size += kSizeOfLong;
      else
        size += i.second.data_type;
    }
    return size;
  }

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
  };

  void addFreePage(size_t page_id)
  {
    database_schema_.free_page_deque.push_back(page_id);
  };

  void updateDatabaseSchema()
  {
    size_t page_num = (getSize(database_schema_) - 1) / kPageSize + 1;
    while (page_num < database_schema_.page_vector.size())
    {
      database_schema_.page_vector.push_back(getFreePage());
      page_num = (getSize(database_schema_) - 1) / kPageSize + 1;
    }
    std::vector<PagePtr> page_ptr_vector;
    for (auto &&i : database_schema_.page_vector)
    {
      page_ptr_vector.push_back(buffer_pool_.getPage(i));
    }
    Stream stream(page_ptr_vector);
    stream << database_schema_;
    for (size_t i = 0; i < page_ptr_vector.size(); ++i)
      file_system_.write(database_schema_.page_vector[i], page_ptr_vector[i]);
  }

  size_t splitFullPage(size_t page_id, char *middle_key);

  void selectRecursive(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &, const std::unordered_set<std::string> &, const std::vector<Node> &select_expr_vector, size_t limit);
  void selectRecursiveAux(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, std::unordered_set<std::string> table_set, std::unordered_set<std::string>, const std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column, const std::vector<Node> &select_expr_vector, size_t limit);
  void deleteRecursive(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &, const std::unordered_set<std::string> &, const std::unordered_set<std::string> &, std::unordered_map<std::string, size_t> &table_id_page_map);
  void deleteRecursiveAux(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, std::unordered_set<std::string> table_set, std::unordered_set<std::string>, const std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column, const std::unordered_set<std::string> &, std::unordered_map<std::string, size_t> &table_id_page_map, std::unordered_map<std::string, size_t> table_id_map);
  std::pair<IndexSchema, std::pair<Token, Token>> getCondition(std::vector<Node> &expr_vector)
  {
    std::unordered_map<IndexSchema, std::pair<Token, Token>, MyIndexSchemaHashFunction, MyIndexSchemaEqualFunction> index_condition_map;
    for (auto &i : expr_vector)
    {
      if (isIndexCondition(i))
      {
        const Node &name_node = i.children.front();
        IndexSchema index_schema;
        if (database_schema_.table_schema_map[name_node.children.front().token.str].column_schema_map[name_node.children.back().token.str].index_schema.root_page_id != -1)
          index_schema = database_schema_.table_schema_map[name_node.children.front().token.str].column_schema_map[name_node.children.back().token.str].index_schema;
        else
          index_schema = database_schema_.table_schema_map[name_node.children.front().token.str].index_schema_map[{name_node.children.back().token.str}].begin()->second;
        int data_type = database_schema_.table_schema_map[name_node.children.front().token.str].column_schema_map[name_node.children.back().token.str].data_type;
        if (index_schema.root_page_id == -1)
        {
          const auto &iter = database_schema_.table_schema_map[name_node.children.front().token.str].index_schema_map.find({name_node.children.back().token.str});
          if (iter == database_schema_.table_schema_map[name_node.children.front().token.str].index_schema_map.end())
            continue;
          index_schema = (*iter->second.begin()).second;
        }
        std::pair<Token, Token> condition;
        if (index_condition_map.find(index_schema) != index_condition_map.end())
          condition = index_condition_map[index_schema];
        if (data_type == 0)
          convertInt(i.children.back().token);
        else if (data_type > 0)
          convertString(i.children.back().token);
        if ((i.token.token_type == kGreater || i.token.token_type == kGreaterEqual))
        {
          if (condition.first.token_type == kNull)
            condition.first = i.children.back().token;
          else if (data_type == 0 && strncmp(reinterpret_cast<const char *>(&i.children.back().token.num), reinterpret_cast<const char *>(&condition.first.num), kSizeOfSizeT) > 0)
            condition.first = i.children.back().token;
          else if (data_type > 0 && i.children.back().token.str > condition.first.str)
            condition.first = i.children.back().token;
        }
        else if ((i.token.token_type == kLess || i.token.token_type == kLessEqual))
        {
          if (condition.second.token_type == kNull)
            condition.second = i.children.back().token;
          else if (data_type == 0 && strncmp(reinterpret_cast<const char *>(&i.children.back().token.num), reinterpret_cast<const char *>(&condition.second.num), kSizeOfSizeT) < 0)
            condition.second = i.children.back().token;
          else if (data_type > 0 && i.children.back().token.str < condition.first.str)
            condition.second = i.children.back().token;
        }
        else if (i.token.token_type == kEqual)
        {
          condition.first = condition.second = i.children.back().token;
        }
        index_condition_map[index_schema] = condition;
      }
    }
    if (index_condition_map.empty())
      return {IndexSchema(), {Token(kNull), Token(kNull)}};
    else
      return {index_condition_map.begin()->first, index_condition_map.begin()->second};
  }

  bool isIndexCondition(Node &node)
  {
    if (node.token.token_type != kGreater && node.token.token_type != kGreaterEqual && node.token.token_type != kLess && node.token.token_type != kLessEqual && node.token.token_type != kEqual)
      return false;
    Node &left_node = node.children.front();
    Node &right_node = node.children.back();
    if (left_node.token.token_type == kName && (right_node.token.token_type == kNull || right_node.token.token_type == kNum || right_node.token.token_type == kString))
    {
      if (database_schema_.table_schema_map[left_node.children.front().token.str].column_schema_map[left_node.children.back().token.str].index_schema.root_page_id != -1 || database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.find({left_node.children.back().token.str}) != database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.end())
        return true;
      else
        return false;
    }
    else if (right_node.token.token_type == kName && (left_node.token.token_type == kNull || left_node.token.token_type == kNum || left_node.token.token_type == kString))
    {
      std::swap(right_node, left_node);
      if (database_schema_.table_schema_map[left_node.children.front().token.str].column_schema_map[left_node.children.back().token.str].index_schema.root_page_id != -1 || database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.find({left_node.children.back().token.str}) != database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.end())
        return true;
      else
        return false;
    }
    else
      return false;
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
