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
  PagePtr getPage(size_t page_num)
  {
    return file_system_.read(page_num);
  }

private:
  BufferPool():file_system_(FileSystem::getInstance()) {}
  FileSystem& file_system_;
};

#endif