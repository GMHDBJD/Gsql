#ifndef TOKEN_H_
#define TOKEN_H_

#include <string>
#include <vector>

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
    kReferences,
    kForeign,
    kIndex,
    kValues,
    kInto,
    kFrom,
    kAlter,
    kJoin,
    kWhere,
    kUse,
    kLimit,
    kAdd,
    kColumn,
    kShow,
    kOn,
    kKey,
    kDatabases,
    kTables,
    kDefault,
    kExplain,
    kUnique,

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
    kMod,
    kLeftParenthesis,
    kRightParenthesis,
    kShiftLeft,
    kShiftRight,
    kBitsExclusiveOr,
    kComma,
    kSemicolon,
    kBitsAnd,
    kBitsOr,
    kBitsNot,
    kNotEqual,

    kNum,
    kString,

    kNone,

    kColumns,
    kNames,
    kName,
    kNotNull,
    kValue,
    kExpr,
    kExprs,
    kJoins,
    kExit,
    kStr,
};

struct Token
{
    Token(TokenType t = kNone, const std::string &s = "", long n = 0) : token_type(t), str(s), num(n){};
    TokenType token_type;
    long num;
    std::string str;
    operator bool()
    {
        return token_type != kNone;
        std::vector<int> v;
    }
};

#endif
