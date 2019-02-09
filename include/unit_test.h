#ifndef LIST_TEST_HPP_
#define LIST_TEST_HPP_

#include "lexer.h"
#include <iostream>

namespace unittest
{
class Test
{
  public:
    void lexerTest()
    {
        std::string arr[] =
            {"kChar", "kInt", "kNull", "kCreate", "kInsert", "kUpdate", "kSet", "kDrop", "kSelect", "kDelete", "kTable", "kDatabase", "kPrimary",
             "kReference", "kIndex", "kValues", "kInto", "kFrom", "kAlter", "kJoin", "kWhere", "kUse", "kAnd", "kNot", "kOr", "kLess", "kGreater",
             "kLessEqual", "kGreateEqual", "kEqual", "kFullStop", "kPlus", "kMinus", "kMultiply", "kDivide", "kLeftParenthesis", "kRightParenthesis", "kComma",
             "kSemicolon", "kBitsAnd", "kBitsOr", "kNotEqual", "kNum", "kString"};

        std::string str_arr[] = {
            "CREATE DATABASE gsql;",
            "USE gsql;",
            "CREATE TABLE test(id INT PRIMARY,val INT NOT NULL, name CHAR REFERENCE other.name);",
            "INSERT INTO TABLE test VALUES(1,null,\"fsd\");",
            "SELECT * FROM test WHERE id>=5 AND id<=9;",
            "UPDATE test SET id=id&1;",
            "DELECT FORM test WHERE id>6;",
            "DROP table test;",
            "SELECT * FROM test JOIN other ON test.name=other.name;",
            "DROP DATABASE gsql;"};
        std::queue<Token> token_queue;
        Lexer lexer;
        for (int i = 0; i < 10; ++i)
        {
            lexer.lex(str_arr[i], &token_queue);
            std::cout << str_arr[i] << std::endl;
            while (!token_queue.empty())
            {
                Token token = token_queue.front();
                token_queue.pop();
                std::cout << arr[token.token_type] << " " << token.num << " " << token.str << std::endl;
            }
            std::cout << std::endl;
        }
    }
};
} // namespace unittest

#endif