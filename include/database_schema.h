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

class MySetHashFunction
{
public:
    template <typename T>
    size_t operator()(const std::unordered_set<T> &t) const
    {
        size_t result = 0;
        for (auto &&i : t)
        {
            result += std::hash<T>()(i);
        }
        return result;
    }
};

struct Settings
{
    std::string database_dir;
};

struct IndexSchema
{
    size_t root_page_id = -1;
    std::string column_name;
};

class MyIndexSchemaHashFunction
{
public:
    size_t operator()(const IndexSchema &index_schema) const
    {
        return std::hash<size_t>()(index_schema.root_page_id);
    }
};

class MyIndexSchemaEqualFunction
{
public:
    bool operator()(const IndexSchema &lhs, const IndexSchema &rhs) const
    {
        return lhs.root_page_id == rhs.root_page_id;
    }
};

struct ColumnSchema
{
    int data_type = 0;
    bool not_null = false;
    bool null_default = true;
    bool unique = false;
    IndexSchema index_schema;
    std::string default_value;
    std::string reference_table_name;
    std::string reference_column_name;
};

struct TableSchema
{
    size_t root_page_id;
    size_t max_id;
    std::vector<std::string> column_order_vector;
    std::unordered_map<std::string, ColumnSchema> column_schema_map;
    std::unordered_set<std::string> primary_set;
    std::unordered_map<std::unordered_set<std::string>, std::unordered_map<std::string, IndexSchema>, MySetHashFunction> index_schema_map;
    std::unordered_map<std::string, std::unordered_set<std::string>> index_column_map;
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