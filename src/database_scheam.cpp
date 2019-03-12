#include "database_schema.h"
#include "stream.h"

std::vector<PagePtr> DatabaseSchema::toPage()
{
    Page *page_arr = new Page[page_vector.size()];
    Stream stream{*page_arr, kPageSize};
    stream << *this;
    std::vector<PagePtr> page_ptr_vector;
    for (size_t i = 0; i < page_vector.size(); ++i)
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

size_t DatabaseSchema::size() const
{
    return getSize(*this);
}