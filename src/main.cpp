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
    GDBE &gdbe = GDBE::getInstance();
    Result result;

    while (true)
    {
        try
        {
            str = shell.getInput();
            std::clock_t start = std::clock();
            token_queue = lexer.lex(str);
            syntax_tree = parser.parse(std::move(token_queue));
            gdbe.exec(std::move(syntax_tree));
            double duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
            while ((result = gdbe.getResult()))
            {
                shell.showResult(result);
                if (result.type == kExitResult)
                    return 0;
            }
            shell.showClock(duration);
        }
        catch (const Error &error)
        {
            shell.showError(error);
        }
    }

    return 0;
}
