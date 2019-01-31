#ifndef BUFFER_POOL_H_
#define BUFFER_POOL_H_

class BufferPool
{
public:
  BufferPool(const BufferPool &) = delete;
  BufferPool &operator=(BufferPool) = delete;
  BufferPool(BufferPool &&) noexcept = delete;
  BufferPool();
  ~BufferPool();
  static BufferPool &getInstance();

private:
};

#endif