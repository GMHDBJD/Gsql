#ifndef BUFFER_POOL_H_
#define BUFFER_POOL_H_
#include <cstddef>
#include <memory>
#include "filesystem.h"

class BufferPool
{
public:
  BufferPool(const BufferPool &) = delete;
  BufferPool &operator=(BufferPool) = delete;
  BufferPool(BufferPool &&) noexcept = delete;
  BufferPool(){}
  ~BufferPool(){}
  PagePtr getPage(size_t page_num)
  {
    return file_system.read(page_num);
  }

private:
};

#endif