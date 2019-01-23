#ifndef LEXER_H_
#define LEXER_H_

#include "token.h"
#include <string>
#include <queue>

class Lexer{
    public:
        Lexer()=default;
        Lexer(const Lexer&)=delete;
        Lexer(Lexer&&)=delete;
        Lexer& operator=(Lexer)=delete;
        ~Lexer()=default;
        int lex(const std::string&,std::queue<Token>*);
};

#endif
