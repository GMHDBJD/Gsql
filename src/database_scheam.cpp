#include "database_schema.h"
#include "stream.h"

std::vector<PagePtr> DatabaseSchema::toPage()
{
    size_t total_size = getSize(*this);
    size_t size = (total_size - 1) / kPageSize + 1;
    Page *page_arr = new Page[size];
    Stream stream{*page_arr, kPageSize};
    stream << *this;
    std::vector<PagePtr> page_ptr_vector;
    for (size_t i = 0; i < (total_size / kPageSize + 1); ++i)
    {
        PagePtr page_ptr(page_arr[i]);
        page_ptr_vector.push_back(page_ptr);
    }
    return page_ptr_vector;
}

void DatabaseSchema::pageTo(const std::vector<PagePtr> &page_ptr_vector)
{
    std::vector<char *> page_pointer_vector;
    for (auto &&i : page_ptr_vector)
        page_pointer_vector.push_back(i.get());
    Stream stream(page_pointer_vector, kPageSize);
    stream >> *this;
}