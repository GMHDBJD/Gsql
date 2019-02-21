#ifndef ERROR_H_
#define ERROR_H_
#include <stdexcept>
#include <string>

enum ErrorType{
    kSqlError,
};

class Error : public std::exception
{
  public:
    Error(ErrorType et, const std::string &em) : std::exception(), error_type_(et), error_message_(em) {}
    const char *  what() const noexcept override
    {
        return error_message_.c_str();
    };
    ErrorType getErrorCode()
    {
        return error_type_;
    }

  private:
    ErrorType error_type_;
    std::string error_message_;
};

#endif