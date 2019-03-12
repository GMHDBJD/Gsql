#include "stream.h"

Stream &operator>>(Stream &stream, DatabaseSchema &database_schema)
{
  stream >> database_schema.page_vector >> database_schema.settings >> database_schema.free_page_deque >> database_schema.table_schema_map >> database_schema.max_page;
  return stream;
}

Stream &operator>>(Stream &stream, Settings &settings)
{
  stream >> settings.database_dir;
  return stream;
}

Stream &operator>>(Stream &stream, TableSchema &table_schema)
{
  stream >> table_schema.root_page_num >> table_schema.column_order_vector >> table_schema.column_schema_map >> table_schema.primary_set >> table_schema.index_schema_map;
  return stream;
}

Stream &operator>>(Stream &stream, ColumnSchema &column_schema)
{
  stream >> column_schema.data_type >> column_schema.not_null >> column_schema.null_default >> column_schema.default_value >> column_schema.reference_table_name >> column_schema.reference_column_name;
  return stream;
}

Stream &operator>>(Stream &stream, IndexSchema &index_schema)
{
  stream >> index_schema.root_page_num >> index_schema.column_name_set;
  return stream;
}

Stream &operator<<(Stream &stream, const DatabaseSchema &database_schema)
{
  stream << database_schema.page_vector << database_schema.settings << database_schema.free_page_deque << database_schema.table_schema_map << database_schema.max_page;
  return stream;
}

Stream &operator<<(Stream &stream, const Settings &settings)
{
  stream << settings.database_dir;
  return stream;
}

Stream &operator<<(Stream &stream, const TableSchema &table_schema)
{
  stream << table_schema.root_page_num << table_schema.column_order_vector << table_schema.column_schema_map << table_schema.primary_set << table_schema.index_schema_map;
  return stream;
}

Stream &operator<<(Stream &stream, const ColumnSchema &column_schema)
{
  stream << column_schema.data_type << column_schema.not_null << column_schema.null_default << column_schema.default_value << column_schema.reference_table_name << column_schema.reference_column_name;
  return stream;
}

Stream &operator<<(Stream &stream, const IndexSchema &index_schema)
{
  stream << index_schema.root_page_num << index_schema.column_name_set;
  return stream;
}

Stream &operator>>(Stream &stream, std::string &data)
{
  size_t size = 0;
  stream >> size;
  std::string temp_string;
  temp_string.resize(size);
  for (size_t i = 0; i < size; ++i)
    stream >> temp_string[i];
  data.swap(temp_string);
  return stream;
}

Stream &operator<<(Stream &stream, const std::string &data)
{
  stream << data.size();
  for (auto &&i : data)
    stream << i;
  return stream;
}

size_t getSize(const std::string &data)
{
  return sizeof(size_t) + data.size() * kSizeOfChar;
}

size_t getSize(const DatabaseSchema &database_schema)
{
  return getSize(database_schema.page_vector) + getSize(database_schema.settings) + getSize(database_schema.free_page_deque) + getSize(database_schema.table_schema_map) + getSize(database_schema.max_page);
}

size_t getSize(const TableSchema &table_schema)
{
  return getSize(table_schema.root_page_num) + getSize(table_schema.column_order_vector) + getSize(table_schema.column_schema_map) + getSize(table_schema.primary_set) + getSize(table_schema.index_schema_map);
}

size_t getSize(const ColumnSchema &column_schema)
{
  return getSize(column_schema.data_type) + getSize(column_schema.not_null) + getSize(column_schema.null_default) + getSize(column_schema.default_value) + getSize(column_schema.reference_table_name) + getSize(column_schema.reference_column_name);
}

size_t getSize(const IndexSchema &index_schema)
{
  return getSize(index_schema.root_page_num) + getSize(index_schema.column_name_set);
}