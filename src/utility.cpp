#include "utility.h"
#include <utility>
#include <algorithm>
#include "const.h"

std::string getTableName(const Node &name_node, const std::string &database_name)
{
    switch (name_node.children.size())
    {
    case 1:
        return name_node.children.front().token.str;
    case 2:
    case 3:
        if (name_node.children.front().token.str != database_name)
            throw Error(kIncorrectDatabaseNameError, name_node.children.front().token.str);
        return name_node.children[1].token.str;
    default:
        throw Error(kIncorrectNameError, name_node.token.str);
    }
}

std::string getColumnName(const Node &name_node, const std::string &database_name, const std::string &table_name_set)
{
    switch (name_node.children.size())
    {
    case 1:
        return name_node.children.front().token.str;
    case 3:
        if (name_node.children.front().token.str != database_name)
            throw Error(kIncorrectDatabaseNameError, name_node.children.front().token.str);
        if (!table_name_set.empty() && name_node.children[1].token.str != table_name_set)
            throw Error(kIncorrectTableNameError, name_node.children[1].token.str);
        return name_node.children.back().token.str;
    case 2:
        if (!table_name_set.empty() && name_node.children.front().token.str != table_name_set)
            throw Error(kIncorrectTableNameError, name_node.children.front().token.str);
        return name_node.children.back().token.str;
    default:
        throw Error(kIncorrectNameError, name_node.token.str);
    }
}

int doNothing(int count, int key)
{
    return 0;
}

void serilization(const std::vector<Token> &values, const std::vector<size_t> &values_size, char *values_ptr)
{
    size_t pos = 0;
    bool null = true;
    bool notnull = false;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (values[i].token_type == kNull)
        {
            std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, values_ptr + pos);
            pos += kSizeOfBool;
            if (values_size[i] == 0)
                pos += kSizeOfLong;
            else
                pos += values_size[i];
        }
        else
        {
            std::copy(reinterpret_cast<const char *>(&notnull), reinterpret_cast<const char *>(&notnull) + kSizeOfBool, values_ptr + pos);
            pos += kSizeOfBool;
            if (values[i].token_type == kNum)
            {
                std::copy(reinterpret_cast<const char *>(&values[i].num), reinterpret_cast<const char *>(&values[i].num) + kSizeOfLong, values_ptr + pos);
                pos += kSizeOfLong;
            }
            else if (values[i].token_type == kString)
            {
                std::copy(values[i].str.c_str(), values[i].str.c_str() + values[i].str.size() + 1, values_ptr + pos);
                if (values[i].str.size() + 1 < values_size[i])
                {
                    size_t cnt = values_size[i] - values[i].str.size() - 1;
                    memset(values_ptr + pos + values[i].str.size() + 1, '\0', cnt);
                }
                pos += values_size[i];
            }
        }
    }
}

bool isNumber(const std::string &s)
{
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

void convertInt(Token &token)
{
    if (token.token_type == kString)
    {
        if (!isNumber(token.str))
            throw Error(kIncorrectValueError, token.str);
        else
        {
            token.token_type = kNum;
            token.num = std::atol(token.str.c_str());
        }
    }
}

void convertString(Token &token)
{
    if (token.token_type != kNone)
        token.token_type = kString;
}

void check(Node &node, const std::unordered_set<std::string> &table_name_set, const std::string database_name, const DatabaseSchema &database_schema)
{
    if (node.token.token_type == kName)
    {
        if (node.children.size() == 1)
        {
            std::string table_name;
            std::string column_name = node.children.back().token.str;
            for (auto &&i : table_name_set)
            {
                const TableSchema &table_schema = database_schema.table_schema_map.find(i)->second;
                if (table_schema.column_schema_map.find(column_name) != table_schema.column_schema_map.end())
                {
                    if (!table_name.empty())
                        throw Error(kColumnAmbiguousError, column_name);
                    table_name = i;
                }
            }
            if (table_name.empty())
                throw Error(kUnkownColumnError, node.children.back().token.str);
            node.children.clear();
            node.children.emplace_back(Token(kString, table_name));
            node.children.emplace_back(Token(kString, column_name));
        }
        else
        {
            std::string table_name;
            if (node.children.size() == 3)
                table_name = node.children[1].token.str;
            else
                table_name = node.children.front().token.str;
            std::string column_name = getColumnName(node, database_name, table_name);
            if (table_name_set.find(table_name) == table_name_set.end())
                throw Error(kUnkownColumnError, table_name + "." + column_name);
            if (database_schema.table_schema_map.at(table_name).column_schema_map.find(column_name) == database_schema.table_schema_map.at(table_name).column_schema_map.end())
                throw Error(kColumnNotExistError, column_name);
            if (node.children.size() == 3)
                node.children.erase(node.children.begin());
        }
    }
    else
    {
        for (auto &&i : node.children)
        {
            check(i, table_name_set, database_name, database_schema);
        }
    }
}

Node eval(const Node &node, const std::unordered_map<std::string, std::unordered_map<std::string, Token>> &table_column_value_map, bool remain)
{
    switch (node.token.token_type)
    {
    case kAnd:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num && right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kNot:
    {
        Node next_node = eval(node.children.front(), table_column_value_map, remain);
        convertInt(next_node.token);
        if (next_node.token.token_type == kNum)
        {
            long result = !next_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (next_node.token.token_type == kNull)
            return next_node;
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(next_node);
            return new_node;
        }
    }
    case kOr:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num || right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kNum && right_node.token.token_type == kNull)
            return Node(Token(kNum, std::to_string(left_node.token.num != 0), left_node.token.num != 0));
        else if (left_node.token.token_type == kNull && right_node.token.token_type == kNum)
            return Node(Token(kNum, std::to_string(right_node.token.num != 0), right_node.token.num != 0));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kLess:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num < right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kString && right_node.token.token_type == kString)
        {
            long result = left_node.token.str != right_node.token.str;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kGreater:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num > right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kString && right_node.token.token_type == kString)
        {
            long result = left_node.token.str != right_node.token.str;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kLessEqual:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num <= right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kString && right_node.token.token_type == kString)
        {
            long result = left_node.token.str != right_node.token.str;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kGreaterEqual:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num >= right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kString && right_node.token.token_type == kString)
        {
            long result = left_node.token.str != right_node.token.str;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kEqual:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num == right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kString && right_node.token.token_type == kString)
        {
            long result = left_node.token.str == right_node.token.str;
            return Node(Token(kNum, std::to_string(result), result));
        }

        else if (!remain)
            throw Error(kOperationError, "");

        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kNotEqual:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num != right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kString && right_node.token.token_type == kString)
        {
            long result = left_node.token.str != right_node.token.str;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kPlus:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num + right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kMinus:
    {
        if (node.children.size() == 2)
        {
            Node left_node = eval(node.children.front(), table_column_value_map, remain);
            Node right_node = eval(node.children.back(), table_column_value_map, remain);
            convertInt(left_node.token);
            convertInt(right_node.token);
            if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
            {
                long result = left_node.token.num - right_node.token.num;
                return Node(Token(kNum, std::to_string(result), result));
            }
            else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
                return Node(Token(kNull, "NULL"));
            else if (!remain)
                throw Error(kOperationError, "");
            else
            {
                Node new_node{node.token};
                new_node.children.push_back(left_node);
                new_node.children.push_back(right_node);
                return new_node;
            }
        }
        else
        {
            Node next_node = eval(node.children.front(), table_column_value_map, remain);
            convertInt(next_node.token);
            if (next_node.token.token_type == kNum)
            {
                long result = -next_node.token.num;
                return Node(Token(kNum, std::to_string(result), result));
            }
            else if (next_node.token.token_type == kNull)
                return Node(Token(kNull, "NULL"));
            else if (!remain)
                throw Error(kOperationError, "");
            else
            {
                Node new_node{node.token};
                new_node.children.push_back(next_node);
                return new_node;
            }
        }
    }
    case kMultiply:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num * right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kDivide:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num / right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kMod:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num % right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kShiftLeft:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num << right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kShiftRight:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num >> right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kBitsExclusiveOr:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num ^ right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kBitsAnd:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num & right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kBitsOr:
    {
        Node left_node = eval(node.children.front(), table_column_value_map, remain);
        Node right_node = eval(node.children.back(), table_column_value_map, remain);
        convertInt(left_node.token);
        convertInt(right_node.token);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            long result = left_node.token.num | right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(left_node);
            new_node.children.push_back(right_node);
            return new_node;
        }
    }
    case kBitsNot:
    {
        Node next_node = eval(node.children.front(), table_column_value_map, remain);
        convertInt(next_node.token);
        if (next_node.token.token_type == kNum)
        {
            long result = ~next_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (next_node.token.token_type == kNull)
            return next_node;
        else if (!remain)
            throw Error(kOperationError, "");
        else
        {
            Node new_node{node.token};
            new_node.children.push_back(next_node);
            return new_node;
        }
    }
    case kNum:
    case kNull:
    case kString:
        return node;
    case kName:
    {
        const auto &table_iter = table_column_value_map.find(node.children.front().token.str);
        if (table_iter != table_column_value_map.end())
        {
            const auto &column_iter = table_iter->second.find(node.children.back().token.str);
            if (column_iter != table_iter->second.end())
                return Node(column_iter->second);
        }
        if (!remain)
            throw Error(kNameNoValueError, node.children.back().token.str);
        return node;
    }
    default:
        throw Error(kSyntaxTreeError, node.token.str);
    }
}

std::vector<Node> splitConditionVector(const std::vector<Node> &condition_node_vector)
{
    std::vector<Node> condition_vector;
    for (const auto &node : condition_node_vector)
    {
        std::vector<Node> new_expr_vector = splitExpr(node);
        condition_vector.insert(condition_vector.begin(), new_expr_vector.begin(), new_expr_vector.end());
    }
    return condition_vector;
}

std::vector<Node> splitExpr(const Node &node)
{
    if (node.token.token_type == kAnd)
    {
        std::vector<Node> new_expr_vector = splitExpr(node.children.front());
        std::vector<Node> right_expr_vector = splitExpr(node.children.back());
        new_expr_vector.insert(new_expr_vector.begin(), right_expr_vector.begin(), right_expr_vector.end());
        return new_expr_vector;
    }
    else
    {
        std::vector<Node> new_expr_vector{node};
        return new_expr_vector;
    }
}

bool partitionConditionForTable(const std::vector<Node> &condition_vector, std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> *table_condition_set_ptr)
{
    for (const auto &i : condition_vector)
    {
        if (i.token.token_type == kNum && i.token.num == 0)
            return false;
        else if (i.token.token_type == kNull)
            return false;
        else if (i.token.token_type == kString)
            return false;
        std::unordered_set<std::string> table_set;
        getTableSet(i, &table_set);
        (*table_condition_set_ptr)[table_set].push_back(i);
    }
    return true;
}

void getTableSet(const Node &expr_node, std::unordered_set<std::string> *table_set_ptr)
{
    if (expr_node.token.token_type == kName)
    {
        table_set_ptr->insert(expr_node.children.front().token.str);
    }
    else if (expr_node.token.token_type != kNum && expr_node.token.token_type != kNull && expr_node.token.token_type != kString)
    {
        for (auto &&i : expr_node.children)
        {
            getTableSet(i, table_set_ptr);
        }
    }
}

std::unordered_map<std::string, Token> toTokenMap(const char *value, const TableSchema &table_schema, size_t key_size, size_t *id)
{
    size_t pos = 0;
    pos += kSizeOfBool;
    *id = *reinterpret_cast<const size_t *>(value + pos);
    pos += kSizeOfSizeT;
    std::unordered_map<std::string, Token> column_token_map;
    for (const auto &column_name : table_schema.column_order_vector)
    {
        int data_type = table_schema.column_schema_map.at(column_name).data_type;
        if (*reinterpret_cast<const bool *>(value + pos))
        {
            column_token_map[column_name] = Token(kNull);
            pos += kSizeOfBool;
            pos += data_type == 0 ? kSizeOfLong : data_type;
            continue;
        }
        else
            pos += kSizeOfBool;
        if (data_type == 0)
        {
            Token token{kNum};
            long num = *reinterpret_cast<const long *>(value + pos);
            pos += kSizeOfLong;
            token.str = std::to_string(num);
            token.num = num;
            column_token_map[column_name] = token;
        }
        else
        {
            Token token{kString};
            token.str = reinterpret_cast<const char *>(value + pos);
            pos += data_type;
            column_token_map[column_name] = token;
        }
    }
    return column_token_map;
}

std::vector<Token> toTokenResultVector(const char *value, const std::vector<int> data_type_vector, size_t key_size)
{
    size_t pos = 0;
    pos += kSizeOfBool;
    size_t id = *reinterpret_cast<const size_t *>(value + pos);
    pos += kSizeOfSizeT;
    std::vector<Token> token_vector;
    for (const auto &data_type : data_type_vector)
    {
        if (*reinterpret_cast<const bool *>(value + pos))
        {
            token_vector.push_back(Token(kNull));
            pos += kSizeOfBool;
            pos += data_type == 0 ? kSizeOfLong : data_type;
            continue;
        }
        else
            pos += kSizeOfBool;
        if (data_type == 0)
        {
            Token token{kNum};
            long num = *reinterpret_cast<const long *>(value + pos);
            pos += kSizeOfLong;
            token.str = std::to_string(num);
            token.num = num;
            token_vector.push_back(token);
        }
        else
        {
            Token token{kString};
            token.str = reinterpret_cast<const char *>(value + pos);
            pos += data_type;
            token_vector.push_back(token);
        }
    }
    return token_vector;
}

int compareInt(char *lhs, char *rhs, size_t size)
{
    if (!*reinterpret_cast<const bool *>(lhs) && !*reinterpret_cast<const bool *>(rhs))
    {
        long lhs_value = *reinterpret_cast<const long *>(lhs + kSizeOfBool);
        long rhs_value = *reinterpret_cast<const long *>(rhs + kSizeOfBool);
        if (lhs_value == rhs_value)
        {
            if (size == kSizeOfLong + kSizeOfBool)
                return 0;
            else
            {
                return std::memcmp(lhs, rhs, size);
            }
        }
        else if (lhs_value < rhs_value)
            return -1;
        else
            return 1;
    }
    else if (*reinterpret_cast<const bool *>(lhs) && *reinterpret_cast<const bool *>(rhs))
    {
        return 0;
    }
    else if (*reinterpret_cast<const bool *>(lhs))
        return -1;
    else
        return 1;
}

int compareString(char *lhs, char *rhs, size_t size)
{
    return std::memcmp(lhs, rhs, size);
}
