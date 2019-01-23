#include "front_end.h"

int FrontEnd::process(const std::string& sql, SyntaxTree* syntax_tree)
{
    std::queue<Token> token_queue;
    lexer.lex(sql,&token_queue);
    parser.parse(token_queue,syntax_tree);
    return 0;
}
