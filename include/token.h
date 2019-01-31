#ifndef TOKEN_H_
#define TOKEN_H_

#include <string>

enum TokenType{

};

class Token{
    public:
        Token(const std::string&,TokenType);
    private:
        std::string str;
        TokenType token_type;
};

#endif
