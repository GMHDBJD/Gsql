#include "buffer_pool.h"

BufferPool &BufferPool::getInstance()
{
    static BufferPool buffer_pool;
    return buffer_pool;
}

PagePtr BufferPool::getPage(size_t page_id)
{
    if (id_page_map_.find(page_id) != id_page_map_.end())
    {
        page_ptr_list_.push_front(*id_page_map_[page_id]);
        page_ptr_list_.erase(id_page_map_[page_id]);
        id_page_map_[page_id] = page_ptr_list_.begin();
    }
    else
    {
        if (page_ptr_list_.size() == kMaxSize)
        {
            auto last_page_ptr = page_ptr_list_.back();
            page_ptr_list_.pop_back();
            id_page_map_.erase(last_page_ptr->page_id);
        }
        PagePtr page_ptr(new Page);
        page_ptr->page_id = page_id;
        file_system_.read(page_id, page_ptr);
        page_ptr_list_.push_front(page_ptr);
        id_page_map_[page_id] = page_ptr_list_.begin();
    }
    return page_ptr_list_.front();
}