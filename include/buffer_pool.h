#ifndef BUFFER_POOL_H_
#define BUFFER_POOL_H_
#include <cstddef>
#include <memory>
#include "file_system.h"

class BufferPool
{
public:
  BufferPool(const BufferPool &) = delete;
  BufferPool &operator=(BufferPool) = delete;
  BufferPool(BufferPool &&) noexcept = delete;
  ~BufferPool() {}
  static BufferPool &getInstance();
  PagePtr getPage(size_t page_id, bool allocate = false)
  {
    if (allocate)
      return PagePtr(new Page);
    else
    {
      PagePtr page_ptr(new Page);
      file_system_.read(page_id, page_ptr);
      return page_ptr;
    }
  }

private:
  BufferPool() : file_system_(FileSystem::getInstance()) {}
  FileSystem &file_system_;
};

#endif