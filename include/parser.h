#ifndef PARSER_H_
#define PARSER_H_
#include "syntax_tree.h"
#include <queue>
#include "token.h"

class Parser{
    public:
        Parser()=default;
        Parser(const Parser&)=delete;
        Parser(Parser&&)=delete;
        Parser& operator=(Parser)=delete;
        ~Parser()=default;
        int parse(std::queue<Token>&&, SyntaxTree*);
};
#endif
