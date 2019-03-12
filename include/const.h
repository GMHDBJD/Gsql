#ifndef CONST_H_
#define CONST_H_

#include <cstddef>
#include <memory>

constexpr int kPageSize = 16000;
constexpr size_t kSizeOfSizeT = sizeof(size_t);
constexpr size_t kSizeOfInt = sizeof(int);
constexpr size_t kSizeOfBool = sizeof(bool);
constexpr size_t kSizeOfChar = sizeof(char);
constexpr size_t kSizeofPageHeader = sizeof(bool) + sizeof(size_t);
using Page = char[kPageSize];
using PagePtr = std::shared_ptr<Page>;

#endif