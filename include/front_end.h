#ifndef FRONTEND_H_
#define FRONTEND_H_


#include "lexer.h"
#include "parser.h"

class FrontEnd{
    public:
        FrontEnd()=default;
        ~FrontEnd()=default;
        FrontEnd(const FrontEnd&)=delete;
        FrontEnd(FrontEnd&&)=delete;
        FrontEnd& operator=(FrontEnd)=delete;
        int Process(const std::string&,SyntaxTree*);
    private:
        Parser parser_;
        Lexer lexer_;
};

#endif
