#include "b_plus_tree.h"
#include "gdbe.h"

bool pageIsFull(const PageSchema &page_schema)
{
    return (kPageSize - kOffsetOfPageHeader) / (page_schema.key_size + page_schema.value_size) <= page_schema.size;
}

bool pageIsMinimum(const PageSchema &page_schema)
{
    return kOffsetOfPageHeader + (2 * page_schema.size + 1) * (page_schema.key_size + page_schema.value_size) < kPageSize;
}

PageSchema getPageSchema(size_t page_id)
{
    BufferPool &buffer_pool = BufferPool::getInstance();
    PageSchema page_schema;
    page_schema.page_ptr = buffer_pool.getPage(page_id);
    page_schema.page_buffer = page_schema.page_ptr->buffer;
    page_schema.leaf = *reinterpret_cast<const bool *>(page_schema.page_buffer + kOffsetOfLeaf);
    page_schema.size = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfSize);
    page_schema.left_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfLeftPageId);
    page_schema.right_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfRightPageId);
    page_schema.key_size = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfKeySize);
    page_schema.index_size = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfIndexSize);
    page_schema.value_size = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfValueSize);
    page_schema.total_size = kOffsetOfPageHeader + page_schema.size * (page_schema.key_size + page_schema.value_size);
    page_schema.page_id = page_id;
    return page_schema;
}

size_t BPlusTreeInsert(size_t page_id, char *key, char *value, bool unique, size_t *root_page_id)
{
    FileSystem &file_system = FileSystem::getInstance();
    PageSchema page_schema = getPageSchema(page_id);
    if (pageIsFull(page_schema))
    {
        PageSchema new_page_schema(false, 0, -1, -1, page_schema.key_size, page_schema.index_size, page_schema.value_size);
        char *left_key = page_schema.page_buffer + kOffsetOfPageHeader;
        char *middle_key = new char[page_schema.key_size];
        size_t right_child_page_id = splitFullPage(page_id, middle_key);
        size_t new_page_id = createNewPage(new_page_schema);
        new_page_schema = getPageSchema(new_page_id);
        std::copy(left_key, left_key + page_schema.key_size, new_page_schema.page_buffer + kOffsetOfPageHeader);
        std::copy(reinterpret_cast<const char *>(&page_schema.page_id), reinterpret_cast<const char *>(&page_schema.page_id) + kSizeOfSizeT, new_page_schema.page_buffer + kOffsetOfPageHeader + new_page_schema.key_size);
        std::copy(middle_key, middle_key + page_schema.key_size, new_page_schema.page_buffer + kOffsetOfPageHeader + page_schema.key_size + kSizeOfSizeT);
        std::copy(reinterpret_cast<const char *>(&right_child_page_id), reinterpret_cast<const char *>(&right_child_page_id) + kSizeOfSizeT, new_page_schema.page_buffer + kOffsetOfPageHeader + kSizeOfSizeT + new_page_schema.key_size * 2);
        new_page_schema.size += 2;
        std::copy(reinterpret_cast<const char *>(&new_page_schema.size), reinterpret_cast<const char *>(&new_page_schema.size) + kSizeOfSizeT, new_page_schema.page_buffer + kOffsetOfSize);
        *root_page_id = new_page_id;
        page_id = new_page_id;
        file_system.write(page_id, new_page_schema.page_ptr);
    }
    return insertNonFullPage(page_id, key, value, unique);
}

bool BPlusTreeSearch(size_t page_id, char *key, bool is_index)
{
    PageSchema page_schema = getPageSchema(page_id);
    size_t pos = kOffsetOfPageHeader;
    size_t compare_size = is_index ? page_schema.index_size : page_schema.key_size;
    while (pos < page_schema.total_size && std::memcmp(key, page_schema.page_buffer + pos, compare_size) >= 0)
        pos += page_schema.key_size + page_schema.value_size;
    if (page_schema.leaf)
        return pos > kOffsetOfPageHeader && std::memcmp(key, page_schema.page_buffer + pos - page_schema.key_size - page_schema.value_size, compare_size) == 0;
    else
    {
        if (pos == kOffsetOfPageHeader)
            return false;
        else
        {
            size_t child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + pos - page_schema.value_size);
            return BPlusTreeSearch(child_page_id, key, is_index);
        }
    }
}

size_t createNewPage(const PageSchema &page_schema)
{
    GDBE &gdbe = GDBE::getInstance();
    size_t new_page_id = gdbe.getFreePage();
    BufferPool &buffer_pool = BufferPool::getInstance();
    FileSystem &file_system = FileSystem::getInstance();
    PagePtr page_ptr = buffer_pool.getPage(new_page_id, true);
    char *new_page_buffer = page_ptr->buffer;
    std::copy(reinterpret_cast<const char *>(&page_schema.leaf), reinterpret_cast<const char *>(&page_schema.leaf) + kSizeOfBool, new_page_buffer + kOffsetOfLeaf);
    std::copy(reinterpret_cast<const char *>(&page_schema.size), reinterpret_cast<const char *>(&page_schema.size) + kSizeOfSizeT, new_page_buffer + kOffsetOfSize);
    std::copy(reinterpret_cast<const char *>(&page_schema.left_page_id), reinterpret_cast<const char *>(&page_schema.left_page_id) + kSizeOfSizeT, new_page_buffer + kOffsetOfLeftPageId);
    std::copy(reinterpret_cast<const char *>(&page_schema.right_page_id), reinterpret_cast<const char *>(&page_schema.right_page_id) + kSizeOfSizeT, new_page_buffer + kOffsetOfRightPageId);
    std::copy(reinterpret_cast<const char *>(&page_schema.key_size), reinterpret_cast<const char *>(&page_schema.key_size) + kSizeOfSizeT, new_page_buffer + kOffsetOfKeySize);
    std::copy(reinterpret_cast<const char *>(&page_schema.index_size), reinterpret_cast<const char *>(&page_schema.index_size) + kSizeOfSizeT, new_page_buffer + kOffsetOfIndexSize);
    std::copy(reinterpret_cast<const char *>(&page_schema.value_size), reinterpret_cast<const char *>(&page_schema.value_size) + kSizeOfSizeT, new_page_buffer + kOffsetOfValueSize);
    file_system.write(new_page_id, page_ptr);
    return new_page_id;
}

size_t insertNonFullPage(size_t page_id, char *key, char *value, bool unique)
{
    FileSystem &file_system = FileSystem::getInstance();
    PageSchema page_schema = getPageSchema(page_id);
    bool null = true;
    size_t pos = kOffsetOfPageHeader;
    while (pos < page_schema.total_size && std::memcmp(key, page_schema.page_buffer + pos, page_schema.key_size) >= 0)
        pos += page_schema.key_size + page_schema.value_size;
    if (page_schema.leaf)
    {
        if (unique && pos > kOffsetOfPageHeader && std::memcmp(key, page_schema.page_buffer + pos - page_schema.key_size - page_schema.value_size, page_schema.index_size) == 0)
            return -1;
        std::copy_backward(page_schema.page_buffer + pos, page_schema.page_buffer + page_schema.total_size, page_schema.page_buffer + page_schema.total_size + page_schema.key_size + page_schema.value_size);
        std::copy(key, key + page_schema.key_size, page_schema.page_buffer + pos);
        std::copy(value, value + page_schema.value_size, page_schema.page_buffer + pos + page_schema.key_size);
        ++page_schema.size;
        std::copy(reinterpret_cast<const char *>(&page_schema.size), reinterpret_cast<const char *>(&page_schema.size) + kSizeOfSizeT, page_schema.page_buffer + kOffsetOfSize);
        file_system.write(page_id, page_schema.page_ptr);
        return page_id;
    }
    else
    {
        size_t child_page_id = -1;
        if (pos == kOffsetOfPageHeader)
        {
            std::copy(key, key + page_schema.key_size, page_schema.page_buffer + kOffsetOfPageHeader);
            child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfPageHeader + page_schema.key_size);
        }
        else
        {
            child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + pos - page_schema.value_size);
        }
        PageSchema child_page_schema = getPageSchema(child_page_id);
        if (pageIsFull(child_page_schema))
        {
            char *middle_key = new char[child_page_schema.key_size];
            size_t right_child_page_id = splitFullPage(child_page_id, middle_key);
            std::copy_backward(page_schema.page_buffer + pos, page_schema.page_buffer + page_schema.total_size, page_schema.page_buffer + page_schema.total_size + page_schema.key_size + page_schema.value_size);
            std::copy(middle_key, middle_key + page_schema.key_size, page_schema.page_buffer + pos);
            std::copy(reinterpret_cast<const char *>(&child_page_id), reinterpret_cast<const char *>(child_page_id) + kSizeOfSizeT, page_schema.page_buffer + pos - page_schema.value_size);
            std::copy(reinterpret_cast<const char *>(&right_child_page_id), reinterpret_cast<const char *>(right_child_page_id) + kSizeOfSizeT, page_schema.page_buffer + pos + page_schema.key_size);
            file_system.write(page_id, page_schema.page_ptr);
            if (std::memcmp(key, middle_key, page_schema.key_size) < 0)
                return insertNonFullPage(child_page_id, key, value, unique);
            else
                return insertNonFullPage(right_child_page_id, key, value, unique);
        }
        else
        {
            return insertNonFullPage(child_page_id, key, value, unique);
        }
    }
}

size_t splitFullPage(size_t page_id, char *middle_key)
{
    FileSystem &file_system = FileSystem::getInstance();
    PageSchema page_schema = getPageSchema(page_id);
    size_t middle = page_schema.size / 2;
    size_t pos = kOffsetOfPageHeader + page_schema.value_size + middle * (page_schema.key_size + page_schema.value_size);
    std::copy(page_schema.page_buffer + pos, page_schema.page_buffer + pos + page_schema.key_size, middle_key);
    PageSchema new_right_page_schema(page_schema.leaf, page_schema.leaf ? page_schema.size - middle : page_schema.size - 1 - middle, page_schema.page_id, page_schema.right_page_id, page_schema.key_size, page_schema.index_size, page_schema.value_size);
    size_t new_right_page_id = createNewPage(new_right_page_schema);
    if (page_schema.leaf)
        std::copy(page_schema.page_buffer + pos, page_schema.page_buffer + page_schema.total_size, new_right_page_schema.page_buffer + kOffsetOfPageHeader + new_right_page_schema.value_size);
    else
        std::copy(page_schema.page_buffer + pos + page_schema.key_size, page_schema.page_buffer + page_schema.total_size, new_right_page_schema.page_buffer + kOffsetOfPageHeader);

    std::copy(reinterpret_cast<const char *>(&middle), reinterpret_cast<const char *>(&middle) + page_schema.key_size, middle_key);
    std::copy(reinterpret_cast<const char *>(&new_right_page_id), reinterpret_cast<const char *>(&new_right_page_id) + kSizeOfSizeT, page_schema.page_buffer + kOffsetOfRightPageId);

    file_system.write(page_id, page_schema.page_ptr);
    file_system.write(new_right_page_id, new_right_page_schema.page_ptr);
    return new_right_page_id;
}

Iterator BPlusTreeSelect(size_t page_id, char *begin_key, char *end_key, bool is_index)
{
    int left = 0, right = 0;
    if (begin_key == nullptr)
        left = -1;
    if (end_key == nullptr)
        right = 1;
    size_t begin_page_id = BPlusTreeTraverse(page_id, begin_key, false, left, is_index);
    size_t end_page_id = BPlusTreeTraverse(page_id, end_key, true, right, is_index);
    return Iterator(begin_page_id, end_page_id);
}

size_t BPlusTreeTraverse(size_t page_id, char *key, bool next, int side, bool is_index)
{
    if (side == 1)
        return -1;
    PageSchema page_schema = getPageSchema(page_id);
    size_t compare_size = is_index ? page_schema.index_size : page_schema.key_size;
    if (page_schema.size == 0)
        return -1;
    else
    {
        if (page_schema.leaf)
        {
            if (!next)
                return page_schema.page_id;
            else if (std::memcmp(key, page_schema.page_buffer + kOffsetOfPageHeader + (page_schema.size - 1) * (page_schema.key_size + page_schema.value_size), compare_size) < 0)
                return page_schema.right_page_id;
            else if (page_schema.right_page_id == -1)
                return -1;
            else
                return BPlusTreeTraverse(page_schema.right_page_id, key, next, side, is_index);
        }
        else
        {
            if (side == -1)
                return BPlusTreeTraverse(*reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfPageHeader), key, next, side, is_index);
            size_t pos = kOffsetOfPageHeader;
            while (pos < page_schema.total_size && std::memcmp(key, page_schema.page_buffer + pos, compare_size) > 0)
                pos += page_schema.value_size + page_schema.key_size;
            if (pos == page_schema.total_size)
                return BPlusTreeTraverse(*reinterpret_cast<const size_t *>(page_schema.page_buffer + page_schema.total_size - page_schema.value_size), key, next, side, is_index);
            if (std::memcmp(key, page_schema.page_buffer + pos, compare_size) == 0)
                return BPlusTreeTraverse(*reinterpret_cast<const size_t *>(page_schema.page_buffer + pos + page_schema.key_size), key, next, side, is_index);
            if (pos == kOffsetOfPageHeader)
                return BPlusTreeTraverse(*reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfPageHeader), key, next, side, is_index);
            return BPlusTreeTraverse(*reinterpret_cast<const size_t *>(page_schema.page_buffer + pos - page_schema.value_size), key, next, side, is_index);
        }
    }
}

void BPlusTreeRemove(size_t page_id)
{
    GDBE &gdbe = GDBE::getInstance();
    PageSchema page_schema = getPageSchema(page_id);
    size_t child_page_id = -1;
    if (!page_schema.leaf)
        child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfPageHeader);
    size_t right_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfRightPageId);
    gdbe.addFreePage(page_id);
    while (right_page_id != -1)
    {
        page_id = right_page_id;
        page_schema = getPageSchema(page_id);
        right_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + kOffsetOfRightPageId);
        gdbe.addFreePage(page_id);
    }
    if (child_page_id != -1)
        return BPlusTreeRemove(child_page_id);
}

void BPlusTreeDelete(size_t page_id, char *key, size_t *root_page_id_ptr)
{
    FileSystem &file_system = FileSystem::getInstance();
    PageSchema page_schema = getPageSchema(page_id);
    size_t pos = kOffsetOfPageHeader;
    if (page_schema.size == 0)
        return;
    while (pos < page_schema.total_size && std::memcmp(key, page_schema.page_buffer + pos, page_schema.key_size) > 0)
        pos += page_schema.value_size + page_schema.key_size;
    if (page_schema.leaf)
    {
        if (pos < page_schema.total_size && std::memcmp(key, page_schema.page_buffer + pos, page_schema.key_size) == 0)
        {
            std::copy(page_schema.page_buffer + pos + page_schema.value_size + page_schema.key_size, page_schema.page_buffer + pos, page_schema.page_buffer + pos);
            --page_schema.size;
            std::copy(reinterpret_cast<const char *>(&page_schema.size), reinterpret_cast<const char *>(&page_schema.size) + kSizeOfSizeT, page_schema.page_buffer + kOffsetOfSize);
            file_system.write(page_schema.page_id, page_schema.page_ptr);
        }
    }
    else
    {
        if (pos < page_schema.total_size && std::memcmp(key, page_schema.page_buffer + pos, page_schema.key_size) == 0)
            pos += page_schema.key_size + page_schema.value_size;
        if (pos == kOffsetOfPageHeader)
            return;
        size_t child_page_pos = pos - page_schema.value_size;
        size_t left_child_page_pos = (pos == kOffsetOfPageHeader + page_schema.value_size + page_schema.key_size ? -1 : child_page_pos - page_schema.value_size - page_schema.key_size);
        size_t right_child_page_pos = (pos == page_schema.total_size ? -1 : child_page_pos + page_schema.value_size + page_schema.key_size);
        size_t child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + child_page_pos);
        PageSchema child_page_schema = getPageSchema(child_page_id);
        if (!pageIsMinimum(child_page_schema))
            return BPlusTreeDelete(child_page_id, key, root_page_id_ptr);
        if (left_child_page_pos != -1)
        {
            size_t left_child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + left_child_page_pos);
            PageSchema left_child_page_schema = getPageSchema(left_child_page_id);
            if (!pageIsMinimum(left_child_page_schema))
            {
                size_t left_child_pos = left_child_page_schema.total_size - left_child_page_schema.key_size - left_child_page_schema.value_size;
                std::copy_backward(child_page_schema.page_buffer + kOffsetOfPageHeader, child_page_schema.page_buffer + child_page_schema.total_size, child_page_schema.page_buffer + child_page_schema.total_size + child_page_schema.key_size + child_page_schema.value_size);
                std::copy(left_child_page_schema.page_buffer + left_child_pos, left_child_page_schema.page_buffer + left_child_pos + left_child_page_schema.key_size + left_child_page_schema.value_size, child_page_schema.page_buffer + kOffsetOfPageHeader);
                std::copy(left_child_page_schema.page_buffer + left_child_pos, left_child_page_schema.page_buffer + left_child_pos + left_child_page_schema.key_size, page_schema.page_buffer + child_page_pos - page_schema.key_size);
                --left_child_page_schema.size;
                std::copy(reinterpret_cast<const char *>(&left_child_page_schema.size), reinterpret_cast<const char *>(&left_child_page_schema.size) + kSizeOfSizeT, left_child_page_schema.page_buffer + kOffsetOfSize);
                ++child_page_schema.size;
                std::copy(reinterpret_cast<const char *>(&child_page_schema.size), reinterpret_cast<const char *>(&child_page_schema.size) + kSizeOfSizeT, child_page_schema.page_buffer + kOffsetOfSize);
                file_system.write(page_schema.page_id, page_schema.page_ptr);
                file_system.write(left_child_page_schema.page_id, left_child_page_schema.page_ptr);
                file_system.write(child_page_schema.page_id, child_page_schema.page_ptr);
                return BPlusTreeDelete(child_page_id, key, root_page_id_ptr);
            }
        }
        if (right_child_page_pos != -1)
        {
            size_t right_child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + right_child_page_pos);
            PageSchema right_child_page_schema = getPageSchema(right_child_page_id);
            if (!pageIsMinimum(right_child_page_schema))
            {
                size_t right_child_pos = kOffsetOfPageHeader;
                std::copy(right_child_page_schema.page_buffer + right_child_pos, right_child_page_schema.page_buffer + right_child_pos + right_child_page_schema.key_size + right_child_page_schema.value_size, child_page_schema.page_buffer + child_page_schema.total_size);
                std::copy(right_child_page_schema.page_buffer + right_child_pos + right_child_page_schema.key_size + right_child_page_schema.value_size, right_child_page_schema.page_buffer + right_child_page_schema.total_size, right_child_page_schema.page_buffer + pos);
                std::copy(right_child_page_schema.page_buffer + right_child_pos, right_child_page_schema.page_buffer + right_child_pos + right_child_page_schema.key_size, page_schema.page_buffer + child_page_pos + page_schema.value_size);
                --right_child_page_schema.size;
                std::copy(reinterpret_cast<const char *>(&right_child_page_schema.size), reinterpret_cast<const char *>(&right_child_page_schema.size) + kSizeOfSizeT, right_child_page_schema.page_buffer + kOffsetOfSize);
                ++child_page_schema.size;
                std::copy(reinterpret_cast<const char *>(&child_page_schema.size), reinterpret_cast<const char *>(&child_page_schema.size) + kSizeOfSizeT, child_page_schema.page_buffer + kOffsetOfSize);
                file_system.write(page_schema.page_id, page_schema.page_ptr);
                file_system.write(right_child_page_schema.page_id, right_child_page_schema.page_ptr);
                file_system.write(child_page_schema.page_id, child_page_schema.page_ptr);
                return BPlusTreeDelete(child_page_id, key, root_page_id_ptr);
            }
        }
        if (left_child_page_pos != -1)
        {
            size_t left_child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + left_child_page_pos);
            PageSchema left_child_page_schema = getPageSchema(left_child_page_id);
            std::copy(child_page_schema.page_buffer + kOffsetOfPageHeader, child_page_schema.page_buffer + child_page_schema.total_size, left_child_page_schema.page_buffer + left_child_page_schema.total_size);
            left_child_page_schema.size += child_page_schema.size;
            std::copy(page_schema.page_buffer + child_page_pos + page_schema.value_size, page_schema.page_buffer + page_schema.total_size, page_schema.page_buffer + child_page_pos - page_schema.key_size);
            --page_schema.size;
            std::copy(reinterpret_cast<const char *>(&left_child_page_schema.size), reinterpret_cast<const char *>(&left_child_page_schema.size) + kSizeOfSizeT, left_child_page_schema.page_buffer + kOffsetOfSize);
            std::copy(reinterpret_cast<const char *>(&page_schema.size), reinterpret_cast<const char *>(&page_schema.size) + kSizeOfSizeT, page_schema.page_buffer + kOffsetOfSize);
            GDBE &gdbe = GDBE::getInstance();
            gdbe.addFreePage(child_page_schema.page_id);
            if (page_schema.size == 0)
            {
                gdbe.addFreePage(page_schema.page_id);
                *root_page_id_ptr = left_child_page_schema.page_id;
            }
            return BPlusTreeDelete(left_child_page_schema.page_id, key, root_page_id_ptr);
        }
        if (right_child_page_pos != -1)
        {
            size_t right_child_page_id = *reinterpret_cast<const size_t *>(page_schema.page_buffer + right_child_page_pos);
            PageSchema right_child_page_schema = getPageSchema(right_child_page_id);
            std::copy(right_child_page_schema.page_buffer + kOffsetOfPageHeader, right_child_page_schema.page_buffer + right_child_page_schema.total_size, child_page_schema.page_buffer + child_page_schema.total_size);
            child_page_schema.size += right_child_page_schema.size;
            std::copy(page_schema.page_buffer + child_page_pos + page_schema.value_size * 2 + page_schema.key_size, page_schema.page_buffer + page_schema.total_size, page_schema.page_buffer + child_page_pos + page_schema.value_size);
            --page_schema.size;
            std::copy(reinterpret_cast<const char *>(&child_page_schema.size), reinterpret_cast<const char *>(&child_page_schema.size) + kSizeOfSizeT, child_page_schema.page_buffer + kOffsetOfSize);
            std::copy(reinterpret_cast<const char *>(&page_schema.size), reinterpret_cast<const char *>(&page_schema.size) + kSizeOfSizeT, page_schema.page_buffer + kOffsetOfSize);
            GDBE &gdbe = GDBE::getInstance();
            gdbe.addFreePage(right_child_page_schema.page_id);
            if (page_schema.size == 0)
            {
                gdbe.addFreePage(page_schema.page_id);
                *root_page_id_ptr = child_page_schema.page_id;
            }
            return BPlusTreeDelete(child_page_schema.page_id, key, root_page_id_ptr);
        }
    }
}