#ifndef GDBE_H_
#define GDBE_H_

#include "syntax_tree.h"
#include "result.h"
#include "error.h"
#include "query_optimizer.h"
#include "buffer_pool.h"
#include "file_system.h"
#include "logger.h"
#include "database_schema.h"
#include "stream.h"

const std::string kDatabaseDir = "database/";

class GDBE
{
public:
  GDBE() : buffer_pool_(BufferPool::getInstance()), file_system_(FileSystem::getInstance()), logger_(Logger::getInstance()) {}
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
  void execInsert(const Node &);
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
  void execDropTable(const Node &);
  void execDropIndex(const Node &){};
  void execExplain(const Node &);
  Node eval(const Node &, const std::string &, const std::unordered_map<std::string, Node> &);
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
      return name_node.childern.back().token.str;
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

  void updateDatabaseSchema()
  {
    size_t page_num = (database_schema_.size() - 1) / kPageSize + 1;
    while (page_num < database_schema_.page_vector.size())
    {
      database_schema_.page_vector.push_back(getFreePage());
      page_num = (database_schema_.size() - 1) / kPageSize + 1;
    }
    std::vector<PagePtr> page_ptr_vector = database_schema_.toPage();
    std::vector<char *> page_vector;
    for (auto &&i : page_ptr_vector)
    {
      page_vector.push_back(i.get());
    }
    /*for (size_t i = 0; i < page_ptr_vector.size(); ++i)
      file_system_.write(database_schema_.page_vector[i], page_ptr_vector[i]);
    PagePtr page_ptr = buffer_pool_.getPage(0);*/
    char *new_page = new Page;
    new_page[0] = 'a';
    new_page[1] = 'b';
    PagePtr page_ptr(new_page);
    file_system_.write(0, page_ptr);
    PagePtr new_page_ptr = buffer_pool_.getPage(0);
    std::cout << memcmp(new_page, new_page_ptr.get(), kPageSize) << std::endl;
  }

  size_t createNewPage()
  {
    size_t new_page_num = getFreePage();
    PagePtr page_ptr = buffer_pool_.getPage(new_page_num, true);
    char *new_page = page_ptr.get();
    bool leaf = true;
    size_t size = 0;
    std::copy(reinterpret_cast<const char *>(&leaf), reinterpret_cast<const char *>(&leaf + kSizeOfBool), new_page);
    std::copy(reinterpret_cast<const char *>(&size), reinterpret_cast<const char *>(&size + kSizeOfSizeT), new_page + kSizeOfBool);
    file_system_.write(new_page_num, page_ptr);
    return new_page_num;
  }

  void insertRootPage(size_t root_page_num, char *key_value, size_t key_size, char *data_value, size_t data_size)
  {
    PagePtr page = buffer_pool_.getPage(root_page_num);
    char *node = page.get();
    bool leaf = *reinterpret_cast<bool *>(node);
    size_t size = *reinterpret_cast<size_t *>(node + kSizeOfBool);
    if (pageIsFull(size, key_size, data_size))
    {
      size_t new_root_page_num = createNewPage();

      // size_t page_num = spiltFullPage(root_page_num);
      //std::copy(reinterpret_cast<const char *>(&page_num), reinterpret_cast<const char *>(&key + kSizeOfSizeT), node + kSizeofPageHeader);
    }
    return insertPage(root_page_num, key_value, key_size, data_value, data_size);
  }

  bool pageIsFull(size_t size, size_t key_size, size_t data_size)
  {
    return (kPageSize - kSizeofPageHeader - data_size) / (key_size + data_size) <= size;
  }

  void insertPage(size_t page_num, char *key_value, size_t key_size, char *data_value, size_t data_size)
  {
    PagePtr page = buffer_pool_.getPage(page_num);
  }

private:
  QueryOptimizer query_optimizer_;
  BufferPool &buffer_pool_;
  FileSystem &file_system_;
  Logger &logger_;
  SyntaxTree syntax_tree_;
  Result result_;
  std::string database_name_;
  DatabaseSchema database_schema_;
};

#endif
