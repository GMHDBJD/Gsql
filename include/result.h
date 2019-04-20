#ifndef RESULT_H_
#define RESULT_H_

#include "syntax_tree.h"
#include <utility>
#include "b_plus_tree.h"

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
    kDeleteResult,
    kExplainResult,
    kExitResult,
    kCreateIndexResult,
    kShowIndexResult,
    kDropIndexResult
};

struct Result
{
public:
    ResultType type;
    std::vector<std::vector<std::string>> string_vector_vector;
    std::vector<size_t> value_size_vector;
    std::vector<int> data_type_vector;
    size_t total_size;
    size_t page_id;
    size_t count;
    Iter iter;
    Iter begin_iter;
    Iter end_iter;
    bool first;
    Result() : type(kNoneResult), page_id(-1), count(0), first(true), iter(-1), begin_iter(-1), end_iter(-1){};
    operator bool()
    {
        return type != kNoneResult;
    }
};

#endif