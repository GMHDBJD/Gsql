#ifndef LEXER_H_
#define LEXER_H_

#include "token.h"
#include "error.h"
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
  std::queue<Token> lex(const std::string &);
  char lookAhead()
  {
    return iter_ == input_ptr_->cend() ? '\0' : *iter_;
  }
  void next()
  {
    if (iter_ != input_ptr_->cend())
      ++iter_;
  }

private:
  const std::string *input_ptr_;
  std::string::const_iterator iter_;
  void setInputPtr(const std::string &input)
  {
    input_ptr_ = &input;
    iter_=input_ptr_->cbegin();
  }
};

#endif
