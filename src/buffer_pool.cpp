#include "buffer_pool.h"

BufferPool &BufferPool::getInstance()
{
    static BufferPool buffer_pool;
    return buffer_pool;
}