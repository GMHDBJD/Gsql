#ifndef LEXER_H_
#define LEXER_H_

#include "token.h"
#include <string>
#include <queue>

class Lexer
{
  public:
    Lexer() = default;
    ~Lexer() = default;
    Lexer(const Lexer &) = delete;
    Lexer(Lexer &&) = delete;
    Lexer &operator=(Lexer) = delete;
    int lex(const std::string &, std::queue<Token> *);
    char lookAhead()
    {
        return iter_ == input_ptr->cend() ? '\0' : *iter_;
    }
    void next()
    {
        if(iter_!=input_ptr->cend())
            ++iter_;
    }

  private:
    const std::string *input_ptr;
    std::string::const_iterator iter_;
};

#endif
