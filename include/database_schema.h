#ifndef DATABASE_SCHEMA_H_
#define DATABASE_SCHEMA_H_
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cstring>
#include "buffer_pool.h"

class MyHashFunction
{
  public:
    template <typename T>
    size_t operator()(const T &t) const
    {
        return std::hash<std::string>()(t.name);
    }
};

class MyEqualFunction
{
  public:
    template <typename T>
    bool operator()(const T &lhs, const T &rhs) const
    {
        return lhs.name == rhs.name;
    }
};

struct Settings
{
    std::string database_dir;
};

enum DataType
{
    kIntType,
    kCharType,
};

struct ColumnSchema
{
    DataType data_type = kIntType;
    bool not_null = false;
    std::string reference_table_name;
    std::string reference_column_name;
};

struct IndexSchema
{
    size_t root_page_num;
    std::unordered_set<std::string> column_name_set;
};

struct TableSchema
{
    size_t root_page_num;
    std::unordered_map<std::string, ColumnSchema> column_schema_map;
    std::unordered_set<std::string> primary_set;
    std::unordered_map<std::string, IndexSchema> index_schema_map;
};

struct DatabaseSchema
{
    std::vector<size_t> page_vector;
    Settings settings;
    std::deque<size_t> free_page_deque;
    std::unordered_map<std::string, TableSchema> table_schema_map;
    size_t max_page;
    std::vector<PagePtr> toPage();
    void pageTo(const std::vector<PagePtr> &page_ptr_vector);
    void clear()
    {
        page_vector.clear();
        free_page_deque.clear();
        table_schema_map.clear();
    }
};

#endif