#include "shell.h"
#include "parser.h"
#include "lexer.h"
#include "error.h"
#include "gdbe.h"

int main()
{
    std::string str;
    std::queue<Token> token_queue;
    SyntaxTree syntax_tree;

    Shell shell;
    Lexer lexer;
    Parser parser;
    GDBE gdbe;
    Result result;

    while (true)
    {
        try
        {
            str = shell.getInput();
            token_queue = lexer.lex(str);
            syntax_tree = parser.parse(std::move(token_queue));
            gdbe.exec(std::move(syntax_tree));
            while (result = gdbe.getResult())
            {
                shell.showResult(result);
                if (result.type_ == kExitResult)
                    return 0;
            }
        }
        catch (const Error &error)
        {
            shell.showError(error);
        }
    }

    return 0;
}
