#ifndef B_PLUS_TREE_H_
#define B_PLUS_TREE_H_

#include <cstddef>
#include <iterator>
#include "const.h"
#include "buffer_pool.h"

size_t createNewPage(bool leaf, size_t size, size_t left_page_id, size_t right_page_id, size_t key_size, size_t value_size);

void BPlusTreeInsert(size_t page_id, char *key, char *value, bool unique);

void insertNonFullPage(size_t page_id, char *key, char *value);

bool pageIsFull(size_t size, size_t key_size, size_t value_size);

size_t splitFullPage(size_t page_id, char *middle_key);

class Iter : public std::iterator<std::random_access_iterator_tag, char *>
{
    PagePtr page_ptr_;
    char *page_buffer_;
    size_t pos_;
    bool leaf_;
    size_t size_;
    size_t left_page_id_;
    size_t right_page_id_;
    size_t key_size_;
    size_t value_size_;
    size_t page_id_;

  public:
    Iter(size_t page_id)
    {
        setPage(page_id);
    }
    Iter &setPage(size_t page_id)
    {
        page_id_ = page_id;
        if (page_id != -1)
        {
            page_ptr_ = BufferPool::getInstance().getPage(page_id);
            page_buffer_ = page_ptr_->buffer;
            leaf_ = *(reinterpret_cast<const bool *>(page_buffer_ + kOffsetOfLeaf));
            size_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfSize));
            left_page_id_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfLeftPageId));
            right_page_id_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfRightPageId));
            key_size_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfKeySize));
            value_size_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfValueSize));
            pos_ = kSizeofPageHeader + value_size_;
        }
        return *this;
    }
    Iter &operator++()
    {
        if (page_id_ != -1)
        {
            pos_ += key_size_ + value_size_;
            if (pos_ >= kSizeofPageHeader + value_size_ + size_ * (value_size_ + key_size_))
                setPage(right_page_id_);
        }
        return *this;
    }
    Iter operator--()
    {
        if (page_id_ != -1)
        {
            if (pos_ >= kSizeofPageHeader + value_size_ * 2 + key_size_)
            {
                pos_ -= key_size_ + value_size_;
                return *this;
            }
            else if (left_page_id_ != -1)
            {
                page_ptr_ = BufferPool::getInstance().getPage(right_page_id_);
                page_buffer_ = page_ptr_->buffer;
                leaf_ = *(reinterpret_cast<const bool *>(page_buffer_ + kOffsetOfLeaf));
                size_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfSize));
                left_page_id_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfLeftPageId));
                right_page_id_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfRightPageId));
                key_size_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfKeySize));
                value_size_ = *(reinterpret_cast<const size_t *>(page_buffer_ + kOffsetOfValueSize));
                pos_ = kSizeofPageHeader + value_size_;
                return *this;
            }
            else
            {
                setPage(-1);
                return *this;
            }
        }
        return *this;
    }
    char *operator*()
    {
        if (page_id_ != -1)
        {
            if (pos_ < kSizeofPageHeader + value_size_ + size_ * (value_size_ + key_size_))
                return page_buffer_ + pos_;
            else
                return nullptr;
        }
        else
        {
            return nullptr;
        }
    }
    bool operator==(const Iter &rhs)
    {
        if (page_id_ == -1 && rhs.page_id_ == -1)
            return true;
        else
            return page_id_ == rhs.page_id_ && pos_ == rhs.pos_;
    }
    bool operator!=(const Iter &rhs)
    {
        return !(*this == rhs);
    }
};

class Iterator
{
  public:
    Iter begin()
    {
        return Iter(begin_page_id_);
    }
    Iter end()
    {
        return Iter(end_page_id_);
    }

    Iterator(size_t begin_page_id, size_t end_page_id) : begin_page_id_(begin_page_id), end_page_id_(end_page_id) {}

  private:
    size_t begin_page_id_;
    size_t end_page_id_;
};

size_t BPlusTreeTraverse(size_t page_id, char *key, bool next,int side);

Iterator BPlusTreeSelect(size_t page_id, char *begin_key, char *end_key);
#endif