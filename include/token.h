#ifndef TOKEN_H_
#define TOKEN_H_

#include <string>

enum TokenType
{
    kChar,
    kInt,
    kNull,

    kCreate,
    kInsert,
    kUpdate,
    kSet,
    kDrop,
    kSelect,
    kDelete,
    kTable,
    kDatabase,
    kPrimary,
    kReference,
    kIndex,
    kValues,
    kInto,
    kFrom,
    kAlter,
    kJoin,
    kWhere,
    kUse,

    kAnd,
    kNot,
    kOr,
    kLess,
    kGreater,
    kLessEqual,
    kGreaterEqual,
    kEqual,
    kFullStop,
    kPlus,
    kMinus,
    kMultiply,
    kDivide,
    kLeftParenthesis,
    kRightParenthesis,
    kComma,
    kSemicolon,
    kBitsAnd,
    kBitsOr,
    kNotEqual,

    kNum,
    kString
};


struct Token
{
    Token(TokenType t, const std::string &s = "", int n = 0) : token_type(t), str(s), num(n){};
    TokenType token_type;
    int num;
    std::string str;
};

#endif
