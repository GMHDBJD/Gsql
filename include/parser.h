#ifndef PARSER_H_
#define PARSER_H_
#include "syntax_tree.h"
#include <queue>
#include "token.h"
#include "error.h"

class Parser
{
public:
  Parser() = default;
  Parser(const Parser &) = delete;
  Parser(Parser &&) = delete;
  Parser &operator=(Parser) = delete;
  ~Parser() = default;
  SyntaxTree parse(std::queue<Token>);
  Token lookAhead()
  {
    if (token_queue_.empty())
      return Token(kNone);
    else
      return token_queue_.front();
  }

private:
  SyntaxTree syntax_tree_;
  std::queue<Token> token_queue_;


  Node parseAll();
  Node parseShow();
  Node parseUse();
  Node parseCreate();
  Node parseInsert();
  Node parseSelect();
  Node parseDelete();
  Node parseDrop();
  Node parseUpdate();
  Node parseAlter();
  Node parseColumns();
  Node parseColumn();
  Node parseValue();
  Node parseValues();
  Node parseExprs();
  Node parseExpr();
  Node parseOr();
  Node parseAnd();
  Node parseBitsOr();
  Node parseBitsExclusiveOr();
  Node parseBitsAnd();
  Node parseEqualOrNot();
  Node parseCompare();
  Node parseShift();
  Node parsePlusMinus();
  Node parseMultiplyDivideMod();
  Node parseNotOrBitsNegative();
  Node parseItem();
  Node parseNames();
  Node parseName();
  Node parseJoins();
  Node parseJoin();
  Node parseWhere();
  Node parsePrimary();
  Node *build(Node new_node, Node *parent = nullptr)
  {
    return syntax_tree_.insert(new_node, parent);
  };
  Token next()
  {
    if (!token_queue_.empty())
    {
      Token token = token_queue_.front();
      token_queue_.pop();
      return token;
    }
    else
      return Token(kNone);
  }
  Token match(TokenType token_type)
  {
    if (lookAhead().token_type == token_type)
      return next();
    else
      throw Error(kSqlError, lookAhead().str);
  }
  void errorProcess(const Error &error)
  {
    throw error;
  };
  void setTokenQueue(std::queue<Token> &&token_queue)
  {
    token_queue_ = token_queue;
  }
};
#endif
