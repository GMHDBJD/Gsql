#ifndef LIST_TEST_HPP_
#define LIST_TEST_HPP_

#include "lexer.h"
#include "parser.h"
#include "shell.h"
#include <iostream>
#include <fstream>
#include <queue>

namespace unittest
{
class Test
{
  private:
    std::string sql_arr[17] = {
        "CREATE DATABASE gsql;",
        "SHOW DATABASES;",
        "USE gsql;",
        "CREATE TABLE gsql.test(id INT PRIMARY KEY, test.val INT NOT NULL, name CHAR REFERENCE other.name);",
        "SHOW TABLES;",
        "CREATE INDEX index ON gsql.test(gsql.test.id,val);",
        "SHOW INDEX FROM gsql.test;",
        "INSERT INTO test VALUES(1,NULL,'fsd'),(2,4,NULL);",
        "INSERT INTO gsql.test(test.id,gsql.test.val)VALUES(1+5,NULL);",
        "SELECT * FROM test WHERE id>=5 AND id<=9 LIMIT 100;",
        "SELECT test.id FROM test;",
        "SELECT 9*9-(8+5)=8*4/5 FROM test JOIN other ON test.name=other.name;",
        "UPDATE test SET id=id&1;",
        "DELETE FROM test WHERE id>6;",
        "ALTER TABLE test ADD str CHAR;",
        "DROP TABLE test;",
        "DROP DATABASE gsql;"};

  public:
    void shellTest()
    {
        Shell shell;
        while (true)
        {
            std::cout<<shell.getInput()<<std::endl;
        }
    }
    void lexerTest()
    {
        std::queue<Token> token_queue;
        Lexer lexer;
        for (int i = 0; i < 17; ++i)
        {
            token_queue = lexer.lex(sql_arr[i]);
            std::cout << sql_arr[i] << std::endl;
            while (!token_queue.empty())
            {
                Token token = token_queue.front();
                token_queue.pop();
                if (token.token_type == kNum)
                    std::cout << token.num << " ";
                else if (token.token_type == kString)
                    std::cout << "'" << token.str << "'"
                              << " ";
                else
                    std::cout << token.str << " ";
            }
            std::cout << std::endl;
        }
    }
    void parserTest()
    {
        std::queue<Token> token_queue;
        Lexer lexer;
        Parser parser;
        SyntaxTree syntax_tree;
        std::ofstream fout("test.md");

        for (int i = 0; i < 17; ++i)
        {
            fout << sql_arr[i] << std::endl;
            fout << "\n```mermaid\ngraph TD;\n";
            token_queue = lexer.lex(sql_arr[i]);
            int cnt = 0;
            int c = 0;
            try
            {
                syntax_tree = parser.parse(std::move(token_queue));
                std::queue<Node> q;
                q.push(syntax_tree.root_);
                while (!q.empty())
                {
                    Node current = q.front();
                    q.pop();
                    for (auto &&child : current.childern)
                    {
                        fout << c << "((";
                        if (current.token.token_type == kNum)
                            fout << current.token.num;
                        else
                            fout << current.token.str;
                        fout << "))"
                             << "-->";
                        fout << ++cnt << "((";
                        if (child.token.token_type == kNum)
                            fout << child.token.num;
                        else
                            fout << child.token.str;
                        fout << "))"
                             << ";" << std::endl;
                        q.push(child);
                    }
                    ++c;
                }
                fout << "\n```\n";
            }
            catch (const Error &error)
            {
                std::cout << "error: " << error.what() << " in " << sql_arr[i] << std::endl;
            }
        }

        fout.close();
    }
};
} // namespace unittest

#endif