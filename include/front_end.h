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
        int process(const std::string&,SyntaxTree*);
    private:
        Parser parser;
        Lexer lexer;
};

#endif
