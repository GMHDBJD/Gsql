#ifndef RESULT_H_
#define RESULT_H_

#include "syntax_tree.h"
#include <utility>

enum ResultType
{
    kNoneResult,
    kSelectResult,
    kShowDatabasesResult,
    kCreateDatabaseResult,
    kUseResult,
    kShowTablesResult,
    kDropDatabaseResult,
    kCreateTableResult,
    kDropTableResult,
    kExplainResult,
    kExitResult
};

struct Result
{
    ResultType type_;
    std::vector<std::vector<std::string>> string_vector_vector_;
    operator bool()
    {
        return type_ != kNoneResult;
    }
};

#endif