#include "shell.h"
#include "parser.h"
#include "lexer.h"
#include "gsql.h"

int main()
{
    std::string str;
    std::queue<Token> token_queue;
    SyntaxTree syntax_tree;

    Shell shell;
    Lexer lexer;
    Parser parser;
    Gsql gsql;
    Result result;

    while (true)
    {
        shell.getInput(&str);
        lexer.lex(str, &token_queue);
        parser.parse(std::move(token_queue), &syntax_tree);
        gsql.run(std::move(syntax_tree));
        while(gsql.getResult(&result))
        {
            shell.showResult(result);
            result.clear();
        }
    }

    return 0;
}
