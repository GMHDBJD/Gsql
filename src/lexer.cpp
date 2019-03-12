#include "lexer.h"

std::queue<Token> Lexer::lex(const std::string &input)
{
    setInputPtr(input);
    char c = '\0';
    std::queue<Token> token_queue;
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
                token_queue.push(Token(kCreate, str));
            else if (str == "SELECT")
                token_queue.push(Token(kSelect, str));
            else if (str == "UPDATE")
                token_queue.push(Token(kUpdate, str));
            else if (str == "TABLE")
                token_queue.push(Token(kTable, str));
            else if (str == "INSERT")
                token_queue.push(Token(kInsert, str));
            else if (str == "INTO")
                token_queue.push(Token(kInto, str));
            else if (str == "DROP")
                token_queue.push(Token(kDrop, str));
            else if (str == "USE")
                token_queue.push(Token(kUse, str));
            else if (str == "FROM")
                token_queue.push(Token(kFrom, str));
            else if (str == "SET")
                token_queue.push(Token(kSet, str));
            else if (str == "DELETE")
                token_queue.push(Token(kDelete, str));
            else if (str == "ALTER")
                token_queue.push(Token(kAlter, str));
            else if (str == "JOIN")
                token_queue.push(Token(kJoin, str));
            else if (str == "WHERE")
                token_queue.push(Token(kWhere, str));
            else if (str == "PRIMARY")
                token_queue.push(Token(kPrimary, str));
            else if (str == "AND")
                token_queue.push(Token(kAnd, str));
            else if (str == "OR")
                token_queue.push(Token(kOr, str));
            else if (str == "NOT")
                token_queue.push(Token(kNot, str));
            else if (str == "VALUES")
                token_queue.push(Token(kValues, str));
            else if (str == "NULL")
                token_queue.push(Token(kNull, str));
            else if (str == "INDEX")
                token_queue.push(Token(kIndex, str));
            else if (str == "DATABASE")
                token_queue.push(Token(kDatabase, str));
            else if (str == "REFERENCES")
                token_queue.push(Token(kReferences, str));
            else if (str == "CHAR")
                token_queue.push(Token(kChar, str));
            else if (str == "INT")
                token_queue.push(Token(kInt, str));
            else if (str == "DATABASES")
                token_queue.push(Token(kDatabases, str));
            else if (str == "TABLES")
                token_queue.push(Token(kTables, str));
            else if (str == "LIMIT")
                token_queue.push(Token(kLimit, str));
            else if (str == "SHOW")
                token_queue.push(Token(kShow, str));
            else if (str == "ADD")
                token_queue.push(Token(kAdd, str));
            else if (str == "COLUMN")
                token_queue.push(Token(kColumn, str));
            else if (str == "ON")
                token_queue.push(Token(kOn, str));
            else if (str == "KEY")
                token_queue.push(Token(kKey, str));
            else if (str == "FOREIGN")
                token_queue.push(Token(kForeign, str));
            else if (str == "DEFAULT")
                token_queue.push(Token(kDefault, str));
            else if(str=="EXPLAIN")
                token_queue.push(Token(kExplain,str));
            else
                token_queue.push(Token(kString, str));
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
            token_queue.push(Token(kNum, std::to_string(i), i));
        }
        else if (c == '\'')
        {
            std::string str;
            next();
            while ((c = lookAhead()) != '\'')
            {
                next();
                str += c;
            }
            token_queue.push(Token(kString, str));
            next();
        }
        else if (c == '"')
        {
            std::string str;
            next();
            while ((c = lookAhead()) != '"')
            {
                next();
                str += c;
            }
            token_queue.push(Token(kString, str));
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
                    token_queue.push(Token(kLessEqual, "LESSEQUAL"));
                }
                else if (c == '<')
                {
                    str += c;
                    next();
                    token_queue.push(Token(kShiftLeft, "SHIFTLEFT"));
                }
                else
                    token_queue.push(Token(kLess, "LESS"));
            }
            else if (str == ">")
            {
                if ((c = lookAhead()) == '=')
                {
                    str += c;
                    next();
                    token_queue.push(Token(kGreaterEqual, "GREATEREQUAL"));
                }
                else if (c == '>')
                {
                    str += c;
                    next();
                    token_queue.push(Token(kShiftRight, "SHIFTRIGHT"));
                }
                else
                    token_queue.push(Token(kGreater, "GREATER"));
            }
            else if (str == "!")
            {
                if ((c = lookAhead()) == '=')
                {
                    str += c;
                    next();
                    token_queue.push(Token(kNotEqual, "NOTEQUAL"));
                }
                else
                    token_queue.push(Token(kNot, "NOT"));
            }
            else if (str == "=")
                token_queue.push(Token(kEqual, "EQUAL"));
            else if (str == "&")
            {
                if ((c = lookAhead()) == '&')
                {
                    str += c;
                    next();
                    token_queue.push(Token(kAnd, "AND"));
                }
                else
                    token_queue.push(Token(kBitsAnd, "BITSAND"));
            }
            else if (str == "|")
            {
                if ((c = lookAhead()) == '|')
                {
                    str += c;
                    next();
                    token_queue.push(Token(kOr, "OR"));
                }
                else
                    token_queue.push(Token(kBitsOr, "BITSOR"));
            }
            else if (str == "+")
                token_queue.push(Token(kPlus, "PLUS"));
            else if (str == "-")
                token_queue.push(Token(kMinus, "MINUS"));
            else if (str == "*")
                token_queue.push(Token(kMultiply, "MULTIPLY"));
            else if (str == "/")
                token_queue.push(Token(kDivide, "DIVIDE"));
            else if (str == "(")
                token_queue.push(Token(kLeftParenthesis, "LEFTPARENTHESIS"));
            else if (str == ")")
                token_queue.push(Token(kRightParenthesis, "RIGHTPARENTHESIS"));
            else if (str == ",")
                token_queue.push(Token(kComma, "COMMA"));
            else if (str == ";")
                token_queue.push(Token(kSemicolon, "SEMICOLON"));
            else if (str == ".")
                token_queue.push(Token(kFullStop, "FULLSTOP"));
            else if (str == "^")
                token_queue.push(Token(kBitsExclusiveOr, "BITSEXCLUSIVEOR"));
            else if (str == "%")
                token_queue.push(Token(kMod, "MOD"));
            else if (str == "~")
                token_queue.push(Token(kBitsNot, "BITSNOT"));
            else
                throw Error(kSqlError, str);
        }
    }
    return token_queue;
}
