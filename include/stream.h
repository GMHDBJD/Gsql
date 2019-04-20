#ifndef STREAM_H_
#define STREAM_H_

#include <algorithm>
#include <vector>
#include <cstddef>
#include "error.h"
#include "database_schema.h"

template <typename T>
size_t getSize(const T &data)
{
  return sizeof(T);
}

template <typename T>
size_t getSize(const std::vector<T> &data)
{
  size_t size = sizeof(size_t);
  for (auto &&i : data)
    size += getSize(i);
  return size;
}

template <typename T, typename U, typename C>
size_t getSize(const std::unordered_set<T, U, C> &data)
{
  size_t size = sizeof(size_t);
  for (auto &&i : data)
    size += getSize(i);
  return size;
}

template <typename T, typename U>
size_t getSize(const std::unordered_map<T, U> &data)
{
  size_t size = sizeof(size_t);
  for (auto &&i : data)
    size += getSize(i.first) + getSize(i.second);
  return size;
}

template <typename T>
size_t getSize(const std::deque<T> &data)
{
  size_t size = sizeof(size_t);
  for (auto &&i : data)
    size += getSize(i);
  return size;
}

template <typename T, typename U>
size_t getSize(const std::pair<T, U> &pair)
{
  return getSize(pair.first) + getSize(pair.second);
}

size_t getSize(const std::string &data);

size_t getSize(const DatabaseSchema &database_schema);

size_t getSize(const TableSchema &table_schema);

size_t getSize(const ColumnSchema &column_schema);

size_t getSize(const IndexSchema &index_schema);

class Stream;

template <typename T>
Stream &operator<<(Stream &, const T &data);
template <typename T>
Stream &operator<<(Stream &, const std::vector<T> &data);

struct Stream
{
public:
  Stream(std::vector<PagePtr> page_ptr_vector) : total_size_(page_ptr_vector.size() * kPageSize), total_pos_(0), vector_index_(0), vector_pos_(0)
  {
    for (auto &&i : page_ptr_vector)
    {
      buffer_vector_.push_back(i->buffer);
    }
  }
  void setBuffer(std::vector<PagePtr> page_ptr_vector)
  {
    for (auto &&i : page_ptr_vector)
    {
      buffer_vector_.push_back(i->buffer);
    }
    total_size_ = page_ptr_vector.size() * kPageSize;
    total_pos_ = 0;
    vector_index_ = 0;
    vector_pos_ = 0;
  }

  std::vector<char *> buffer_vector_;
  size_t total_size_;
  size_t total_pos_;
  size_t vector_index_;
  size_t vector_pos_;
};

template <typename T>
Stream &operator<<(Stream &stream, const T &data)
{
  if (stream.total_pos_ + sizeof(T) > stream.total_size_)
    throw Error(kMemoryError, "Memory error");
  size_t data_size = sizeof(T);
  size_t current_copy_size = 0;
  size_t capacy = kPageSize - stream.vector_pos_;
  while (data_size > capacy)
  {
    std::copy(reinterpret_cast<const char *>(&data + current_copy_size), reinterpret_cast<const char *>(&data + current_copy_size + capacy), stream.buffer_vector_[stream.vector_index_] + stream.vector_pos_);
    stream.total_pos_ += capacy;
    data_size -= capacy;
    stream.vector_pos_ = 0;
    ++stream.vector_index_;
    capacy = kPageSize;
  }
  std::copy(reinterpret_cast<const char *>(&data + current_copy_size), reinterpret_cast<const char *>(&data + current_copy_size + data_size), stream.buffer_vector_[stream.vector_index_] + stream.vector_pos_);
  stream.total_pos_ += data_size;
  stream.vector_pos_ += data_size;
  return stream;
}

template <typename T>
Stream &operator<<(Stream &stream, const std::vector<T> &data)
{
  stream << data.size();
  for (auto &&i : data)
    stream << i;
  return stream;
}

template <typename T, typename U, typename C>
Stream &operator<<(Stream &stream, const std::unordered_set<T, U, C> &data)
{
  stream << data.size();
  for (auto &&i : data)
    stream << i;
  return stream;
}

template <typename T, typename U>
Stream &operator<<(Stream &stream, const std::unordered_map<T, U> &data)
{
  stream << data.size();
  for (auto &&i : data)
    stream << i.first << i.second;
  return stream;
}

template <typename T, typename U, typename V>
Stream &operator<<(Stream &stream, const std::unordered_map<T, U, V> &data)
{
  stream << data.size();
  for (auto &&i : data)
    stream << i.first << i.second;
  return stream;
}

template <typename T>
Stream &operator<<(Stream &stream, const std::deque<T> &data)
{
  stream << data.size();
  for (auto &&i : data)
    stream << i;
  return stream;
}

template <typename T, typename U>
Stream &operator<<(Stream &stream, const std::pair<T, U> &pair)
{
  stream << pair.first << pair.second;
  return stream;
}

template <typename T>
Stream &operator>>(Stream &stream, T &data)
{
  if (stream.total_pos_ + sizeof(T) > stream.total_size_)
    throw Error(kMemoryError, "Memory error");
  size_t data_size = sizeof(T);
  size_t current_copy_size = 0;
  size_t capacy = kPageSize - stream.vector_pos_;
  while (data_size > capacy)
  {
    std::copy(reinterpret_cast<const T *>(stream.buffer_vector_[stream.vector_index_] + stream.vector_pos_), reinterpret_cast<const T *>(stream.buffer_vector_[stream.vector_index_] + stream.vector_pos_ + capacy), &data + current_copy_size);
    stream.total_pos_ += capacy;
    data_size -= capacy;
    stream.vector_pos_ = 0;
    ++stream.vector_index_;
    capacy = kPageSize;
  }
  std::copy(reinterpret_cast<const T *>(stream.buffer_vector_[stream.vector_index_] + stream.vector_pos_), reinterpret_cast<const T *>(stream.buffer_vector_[stream.vector_index_] + stream.vector_pos_ + data_size), &data + current_copy_size);
  stream.total_pos_ += data_size;
  stream.vector_pos_ += data_size;
  return stream;
}

template <typename T>
Stream &operator>>(Stream &stream, std::vector<T> &data)
{
  size_t size = 0;
  stream >> size;
  std::vector<T> temp_vector;
  temp_vector.resize(size);
  for (size_t i = 0; i < size; ++i)
    stream >> temp_vector[i];
  data.swap(temp_vector);
  return stream;
}

template <typename T, typename U, typename C>
Stream &operator>>(Stream &stream, std::unordered_set<T, U, C> &data)
{
  size_t size = 0;
  stream >> size;
  data.clear();
  for (size_t i = 0; i < size; ++i)
  {
    T key;
    stream >> key;
    data.insert(key);
  }
  return stream;
}

template <typename T, typename U>
Stream &operator>>(Stream &stream, std::unordered_map<T, U> &data)
{
  size_t size = 0;
  stream >> size;
  data.clear();
  for (size_t i = 0; i < size; ++i)
  {
    T key;
    U value;
    stream >> key >> value;
    data[key] = value;
  }
  return stream;
}

template <typename T, typename U, typename V>
Stream &operator>>(Stream &stream, std::unordered_map<T, U, V> &data)
{
  size_t size = 0;
  stream >> size;
  data.clear();
  for (size_t i = 0; i < size; ++i)
  {
    T key;
    U value;
    stream >> key >> value;
    data[key] = value;
  }
  return stream;
}

template <typename T>
Stream &operator>>(Stream &stream, std::deque<T> &data)
{
  size_t size = 0;
  stream >> size;
  std::deque<T> temp_deque;
  temp_deque.resize(size);
  for (size_t i = 0; i < size; ++i)
    stream >> temp_deque[i];
  data.swap(temp_deque);
  return stream;
}

template <typename T, typename U>
Stream &operator>>(Stream &stream, std::pair<T, U> &pair)
{
  stream >> pair.first >> pair.second;
  return stream;
}

Stream &operator>>(Stream &stream, DatabaseSchema &database_schema);

Stream &operator>>(Stream &stream, TableSchema &table_schema);

Stream &operator>>(Stream &stream, ColumnSchema &column_schema);

Stream &operator>>(Stream &stream, IndexSchema &index_schema);

Stream &operator<<(Stream &stream, const DatabaseSchema &database_schema);

Stream &operator<<(Stream &stream, const TableSchema &table_schema);

Stream &operator<<(Stream &stream, const ColumnSchema &column_schema);

Stream &operator<<(Stream &stream, const IndexSchema &index_schema);

Stream &operator>>(Stream &stream, std::string &data);

Stream &operator<<(Stream &stream, const std::string &data);

Stream &operator<<(Stream &stream, const Settings &settings);

Stream &operator>>(Stream &stream, Settings &settings);
#endif