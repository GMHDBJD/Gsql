#ifndef CONST_H_
#define CONST_H_

#include <cstddef>
#include <memory>

constexpr size_t kPageSize = 16000;
constexpr size_t kSizeOfSizeT = sizeof(size_t);
constexpr size_t kSizeOfInt = sizeof(int);
constexpr size_t kSizeOfBool = sizeof(bool);
constexpr size_t kSizeOfChar = sizeof(char);
constexpr size_t kOffsetOfLeaf = 0;
constexpr size_t kOffsetOfSize = sizeof(bool);
constexpr size_t kOffsetOfLeftPageId = kOffsetOfSize + sizeof(size_t);
constexpr size_t kOffsetOfRightPageId = kOffsetOfLeftPageId + sizeof(size_t);
constexpr size_t kOffsetOfKeySize = kOffsetOfRightPageId + sizeof(size_t);
constexpr size_t kOffsetOfValueSize = kOffsetOfKeySize + sizeof(size_t);
constexpr size_t kSizeofPageHeader = kOffsetOfValueSize + sizeof(size_t);

struct Page
{
    char buffer[kPageSize];
};

using PagePtr = std::shared_ptr<Page>;

#endif