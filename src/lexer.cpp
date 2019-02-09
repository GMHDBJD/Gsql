#include "lexer.h"

int Lexer::lex(const std::string &input, std::queue<Token> *token_queue_ptr)
{
    input_ptr = &input;
    iter_ = input_ptr->cbegin();
    char c = '\0';
    while ((c = lookAhead()) != '\0')
    {
        if (isspace(c))
            next();
        else if (isalpha(c))
        {
            std::string str;
            str += c;
            next();
            while (isalpha(c = lookAhead()) || isdigit(c) || c == '_')
            {
                str += c;
                next();
            }
            if (str == "CREATE")
                token_queue_ptr->push(Token(kCreate));
            else if (str == "SELECT")
                token_queue_ptr->push(Token(kSelect));
            else if (str == "UPDATE")
                token_queue_ptr->push(Token(kUpdate));
            else if (str == "TABLE")
                token_queue_ptr->push(Token(kTable));
            else if (str == "INSERT")
                token_queue_ptr->push(Token(kInsert));
            else if (str == "INTO")
                token_queue_ptr->push(Token(kInto));
            else if (str == "DROP")
                token_queue_ptr->push(Token(kDrop));
            else if (str == "USE")
                token_queue_ptr->push(Token(kUse));
            else if (str == "FROM")
                token_queue_ptr->push(Token(kFrom));
            else if (str == "SET")
                token_queue_ptr->push(Token(kSet));
            else if (str == "DELETE")
                token_queue_ptr->push(Token(kDelete));
            else if (str == "ALTER")
                token_queue_ptr->push(Token(kAlter));
            else if (str == "JOIN")
                token_queue_ptr->push(Token(kJoin));
            else if (str == "WHERE")
                token_queue_ptr->push(Token(kWhere));
            else if (str == "PRIMARY")
                token_queue_ptr->push(Token(kPrimary));
            else if (str == "AND")
                token_queue_ptr->push(Token(kAnd));
            else if (str == "OR")
                token_queue_ptr->push(Token(kOr));
            else if (str == "NOT")
                token_queue_ptr->push(Token(kNot));
            else if (str == "VALUES")
                token_queue_ptr->push(Token(kValues));
            else if (str == "NULL")
                token_queue_ptr->push(Token(kNull));
            else if (str == "INDEX")
                token_queue_ptr->push(Token(kIndex));
            else if (str == "DATABASE")
                token_queue_ptr->push(Token(kDatabase));
            else if (str == "REFERENCE")
                token_queue_ptr->push(Token(kReference));
            else if (str == "CHAR")
                token_queue_ptr->push(Token(kChar));
            else if (str == "INT")
                token_queue_ptr->push(Token(kInt));
            else
                token_queue_ptr->push(Token(kString, str));
        }
        else if (isdigit(c))
        {
            int i = c - '0';
            next();
            while (isdigit((c = lookAhead())))
            {
                i *= 10 + c - '0';
                next();
            }
            token_queue_ptr->push(Token(kNum, "", 0));
        }
        else if (c == '\'')
        {
            std::string str;
            next();
            while ((c = lookAhead()) != '\'')
                str += c;
            token_queue_ptr->push(Token(kString, str, 0));
            next();
        }
        else if (ispunct(c))
        {
            std::string str;
            str += c;
            next();
            if (str == "<")
            {
                if ((c = lookAhead()) == '=')
                {
                    str += c;
                    next();
                    token_queue_ptr->push(Token(kLessEqual));
                }
                else
                    token_queue_ptr->push(Token(kLess));
            }
            else if (str == ">")
            {
                if ((c = lookAhead()) == '=')
                {
                    str += c;
                    next();
                    token_queue_ptr->push(Token(kGreaterEqual));
                }
                else
                    token_queue_ptr->push(Token(kGreater));
            }
            else if (str == "!")
            {
                if ((c = lookAhead()) == '=')
                {
                    str += c;
                    next();
                    token_queue_ptr->push(Token(kNotEqual));
                }
                else
                    token_queue_ptr->push(Token(kNot));
            }
            else if (str == "=")
                token_queue_ptr->push(Token(kEqual));
            else if (str == "&")
            {
                if ((c = lookAhead()) == '&')
                {
                    str += c;
                    next();
                    token_queue_ptr->push(Token(kAnd));
                }
                else
                    token_queue_ptr->push(Token(kBitsAnd));
            }
            else if (str == "|")
            {
                if ((c = lookAhead()) == '|')
                {
                    str += c;
                    next();
                    token_queue_ptr->push(Token(kOr));
                }
                else
                    token_queue_ptr->push(Token(kBitsOr));
            }
            else if (str == "+")
                token_queue_ptr->push(Token(kPlus));
            else if (str == "-")
                token_queue_ptr->push(Token(kMinus));
            else if (str == "*")
                token_queue_ptr->push(Token(kMultiply));
            else if (str == "/")
                token_queue_ptr->push(Token(kDivide));
            else if (str == "(")
                token_queue_ptr->push(Token(kLeftParenthesis));
            else if (str == ")")
                token_queue_ptr->push(Token(kRightParenthesis));
            else if (str == ",")
                token_queue_ptr->push(Token(kComma));
            else if (str == ";")
                token_queue_ptr->push(Token(kSemicolon));
            else if(str==".")
                token_queue_ptr->push(Token(kFullStop));
         }
    }
    return 0;
}
