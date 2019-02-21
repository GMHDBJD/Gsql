#include "shell.h"
#include "parser.h"
#include "lexer.h"
#include "error.h"
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
        try
        {
            str = shell.getInput();
            token_queue = lexer.lex(str);
            syntax_tree = parser.parse(std::move(token_queue));
            gsql.run(std::move(syntax_tree));
            while (result = gsql.getResult())
            {
                shell.showResult(result);
            }
        }
        catch (const Error &error)
        {
            shell.showError(error);
        }
    }

    return 0;
}
