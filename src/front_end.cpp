#include "front_end.h"

int FrontEnd::Process(const std::string& sql, SyntaxTree* syntax_tree)
{
    std::queue<Token> token_queue;
    lexer.Lex(sql,&token_queue);
    parser.Parse(token_queue,syntax_tree);
    return 0;
}
