#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>
#include "syntax_tree.h"
#include "error.h"
#include <vector>


std::string getTableName(const Node &name_node, const std::string &database_name);

std::string getColumnName(const Node &name_node, const std::string &database_name, const std::string &table_name);

int doNothing(int count, int key);

void serilization(const std::vector<Token> &values, const std::vector<size_t> &values_size, char *values_ptr);

void convertInt(Token &token);

void convertString(Token &token);

bool isNumber(const std::string &s);
#endif