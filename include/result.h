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
    kDropIndexResult,
    kInsertResult
};

struct Result
{
public:
    ResultType type;
    std::vector<std::vector<std::string>> string_vector_vector;
    std::vector<size_t> value_size_vector;
    std::vector<int> data_type_vector;
    std::vector<std::string> header;
    size_t total_size;
    size_t page_id;
    size_t count;
    bool start;
    bool first;
    bool last;
    Iter iter;
    Iter begin_iter;
    Iter end_iter;
    Result() : type(kNoneResult), total_size(0), page_id(-1), count(0), start(true), first(true), last(false), iter(-1), begin_iter(-1), end_iter(-1){};
    operator bool()
    {
        return type != kNoneResult;
    }
    void clear()
    {
        type = kNoneResult;
        string_vector_vector.clear();
        value_size_vector.clear();
        data_type_vector.clear();
        header.clear();
        total_size = 0;
        page_id = -1;
        count = 0;
        start = true;
        first = true;
        last = false;
        iter.setPage({-1, kOffsetOfPageHeader});
        begin_iter.setPage({-1, kOffsetOfPageHeader});
        end_iter.setPage({-1, kOffsetOfPageHeader});
    }
};

#endif