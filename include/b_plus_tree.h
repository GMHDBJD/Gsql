#ifndef B_PLUS_TREE_H_
#define B_PLUS_TREE_H_

#include <cstddef>
#include <iterator>
#include "const.h"
#include "buffer_pool.h"

size_t createNewPage(const PageSchema &page_schema);

void BPlusTreeInsert(size_t page_id, char *key, char *value, bool unique, size_t *root_page_id);

void BPlusTreeDelete(size_t page_id, char *key, size_t *root_page_id);

void removeBPlusTree(size_t page_id);

void insertNonFullPage(size_t page_id, char *key, char *value);

bool pageIsFull(const PageSchema &page_schema);

bool pageIsMinimum(const PageSchema &page_schema);

PageSchema getPageSchema(size_t page_id);

size_t splitFullPage(size_t page_id, char *middle_key);

class Iter : public std::iterator<std::random_access_iterator_tag, char *>
{
    PageSchema page_schema_;
    size_t pos_;

public:
    Iter(size_t page_id)
    {
        setPage(page_id);
    }
    Iter &setPage(size_t page_id)
    {
        if (page_id == -1)
            page_schema_.page_id = -1;
        else
        {
            page_schema_ = getPageSchema(page_id);
            pos_ = kOffsetOfPageHeader;
        }
        return *this;
    }
    size_t getPageId() const
    {
        return page_schema_.page_id;
    }
    Iter &operator++()
    {
        if (page_schema_.page_id != -1)
        {
            pos_ += page_schema_.key_size + page_schema_.value_size;
            if (pos_ >= page_schema_.total_size)
                setPage(page_schema_.right_page_id);
        }
        return *this;
    }
    char *operator*()
    {
        if (page_schema_.page_id != -1)
        {
            if (pos_ < page_schema_.total_size)
                return page_schema_.page_buffer + pos_;
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
        if (page_schema_.page_id == -1 && rhs.page_schema_.page_id == -1)
            return true;
        else
            return page_schema_.page_id == rhs.page_schema_.page_id && pos_ == rhs.pos_;
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

size_t BPlusTreeTraverse(size_t page_id, char *key, bool next, int side, bool is_index);

Iterator BPlusTreeSelect(size_t page_id, char *begin_key, char *end_key, bool is_index);
#endif