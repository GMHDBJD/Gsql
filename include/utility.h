#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>
#include "syntax_tree.h"
#include "error.h"
#include <vector>
#include <unordered_set>
#include "database_schema.h"

std::string getTableName(const Node &name_node, const std::string &database_name);

std::string getColumnName(const Node &name_node, const std::string &database_name, const std::string &table_name);

int doNothing(int count, int key);

void serilization(const std::vector<Token> &values, const std::vector<size_t> &values_size, char *values_ptr);

std::unordered_map<std::string, Token> toTokenVector(const char *value, const TableSchema &table_schema, size_t key_size);

std::vector<Token> toTokenResultVector(const char *value, const std::vector<int> data_type_vector, size_t key_size);

void convertInt(Token &token);

void convertString(Token &token);

bool isNumber(const std::string &s);

Node eval(const Node &, const std::unordered_map<std::string, std::unordered_map<std::string, Token>> &, bool);

void check(Node &, const std::unordered_set<std::string> &, const std::string database_name, const DatabaseSchema &database_schema);

std::vector<Node> splitExprVector(const std::vector<Node> &condition_node_vector);

std::vector<Node> splitExpr(const Node &);

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

bool partitionExprVector(const std::vector<Node> &, std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> *);

void getTableSet(const Node &, std::unordered_set<std::string> *);

bool isIndexCondition(const Node &node);
#endif