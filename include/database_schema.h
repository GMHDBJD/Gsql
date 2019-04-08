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

struct ColumnSchema
{
    int data_type = 0;
    bool not_null = false;
    bool null_default = true;
    bool unique = false;
    std::string index_name;
    std::string default_value;
    std::string reference_table_name;
    std::string reference_column_name;
};

struct IndexSchema
{
    size_t root_page_id;
    std::unordered_set<std::string> column_name_set;
};

struct TableSchema
{
    size_t root_page_id;
    size_t max_id;
    std::vector<std::string> column_order_vector;
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
    size_t max_page = 0;
    void clear()
    {
        page_vector.clear();
        free_page_deque.clear();
        table_schema_map.clear();
    }
    void swap(DatabaseSchema &rhs)
    {
        using std::swap;
        swap(page_vector, rhs.page_vector);
        swap(settings, rhs.settings);
        swap(free_page_deque, rhs.free_page_deque);
        swap(table_schema_map, rhs.table_schema_map);
        swap(max_page, rhs.max_page);
    }
};

#endif