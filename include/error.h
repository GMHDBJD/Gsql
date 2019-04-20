#ifndef ERROR_H_
#define ERROR_H_
#include <stdexcept>
#include <string>

enum ErrorType
{
  kSqlError,
  kSyntaxTreeError,
  kDatabaseNotExistError,
  kDatabaseExistError,
  kMemoryError,
  kIncorrectDatabaseNameError,
  kIncorrectNameError,
  kTableExistError,
  kIncorrectTableNameError,
  kTableNotExistError,
  kColumnNotExistError,
  kDuplicateColumnError,
  kAddForeiginError,
  kMultiplePrimaryKeyError,
  kNoDatabaseSelectError,
  kOperationError,
  kInvalidDefaultValueError,
  kColumnCountNotMatchError,
  kIncorrectValueError,
  kIncorrectIntegerValue,
  kNotUniqueTableError,
  kColumnAmbiguousError,
  kUnkownColumnError,
  kNameNoValueError,
  kDuplicateIndexError,
  kIndexExistError,
  kIndexNotExistError,
  kDuplicateEntryError,
  kColumnNotNullError,
  kForeignkeyConstraintError,
  kUnkownTableError
};

class Error : public std::exception
{
public:
  Error(ErrorType et, const std::string &em) : std::exception(), error_type_(et), error_message_(em) {}
  const char *what() const noexcept override
  {
    return error_message_.c_str();
  };
  ErrorType getErrorCode() const
  {
    return error_type_;
  }

private:
  ErrorType error_type_;
  std::string error_message_;
};

#endif