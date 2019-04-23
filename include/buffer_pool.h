#ifndef BUFFER_POOL_H_
#define BUFFER_POOL_H_
#include <cstddef>
#include <memory>
#include <list>
#include <unordered_map>
#include "file_system.h"

class BufferPool
{
public:
  BufferPool(const BufferPool &) = delete;
  BufferPool &operator=(BufferPool) = delete;
  BufferPool(BufferPool &&) noexcept = delete;
  ~BufferPool() {}
  static BufferPool &getInstance();
  PagePtr getPage(size_t page_id);
  void clear()
  {
    page_ptr_list_.clear();
    id_page_map_.clear();
  }

private:
  BufferPool() : file_system_(FileSystem::getInstance()) {}

  FileSystem &file_system_;
  std::list<PagePtr> page_ptr_list_;
  std::unordered_map<int, std::list<PagePtr>::iterator> id_page_map_;
  size_t kMaxSize = 2000;
};

#endif