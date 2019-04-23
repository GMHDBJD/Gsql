#ifndef CONST_H_
#define CONST_H_

#include <cstddef>
#include <memory>

constexpr size_t kPageSize = 16000;
constexpr size_t kSizeOfSizeT = sizeof(size_t);
constexpr size_t kSizeOfInt = sizeof(int);
constexpr size_t kSizeOfBool = sizeof(bool);
constexpr size_t kSizeOfLong = sizeof(long);
constexpr size_t kSizeOfChar = sizeof(char);

constexpr size_t kOffsetOfLeaf = 0;
constexpr size_t kOffsetOfSize = sizeof(bool);
constexpr size_t kOffsetOfLeftPageId = kOffsetOfSize + sizeof(size_t);
constexpr size_t kOffsetOfRightPageId = kOffsetOfLeftPageId + sizeof(size_t);
constexpr size_t kOffsetOfKeySize = kOffsetOfRightPageId + sizeof(size_t);
constexpr size_t kOffsetOfIndexSize = kOffsetOfKeySize + sizeof(size_t);
constexpr size_t kOffsetOfValueSize = kOffsetOfIndexSize + sizeof(size_t);
constexpr size_t kOffsetOfCompareType = kOffsetOfValueSize + sizeof(size_t);
constexpr size_t kOffsetOfPageHeader = kOffsetOfCompareType + sizeof(bool);

struct Page
{
    size_t page_id;
    char buffer[kPageSize];
};

using PagePtr = std::shared_ptr<Page>;

struct PageSchema
{
    bool leaf;
    size_t size;
    size_t left_page_id;
    size_t right_page_id;
    size_t key_size;
    size_t index_size;
    size_t value_size;
    PagePtr page_ptr;
    bool cmp;
    char *page_buffer;
    int (*compare)(char *lhs, char *rhs, size_t);
    size_t total_size;
    size_t page_id;
    PageSchema(bool l, size_t s, size_t left, size_t right, size_t key, size_t index, size_t value, bool c) : leaf(l), size(s), left_page_id(left), right_page_id(right), key_size(key), index_size(index), value_size(value), cmp(c) {}
    PageSchema() = default;
};

#endif