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
  Stream(char *mem, size_t per_size) : total_size_(per_size), total_pos_(0), vector_index_(0), vector_pos_(0), per_size_(per_size)
  {
    memory_vector_.push_back(mem);
  }
  Stream(std::vector<char *> mem_vector, size_t per_size) : total_size_(mem_vector.size() * per_size), memory_vector_(mem_vector), total_pos_(0), vector_index_(0), vector_pos_(0), per_size_(per_size) {}
  void setMemory(std::vector<char *> mem_vector)
  {
    memory_vector_ = mem_vector;
    total_size_ = 0;
    for (auto &&i : mem_vector)
    {
      total_size_ += sizeof(i);
    }
    total_pos_ = 0;
    vector_index_ = 0;
    vector_pos_ = 0;
  }

  std::vector<char *> memory_vector_;
  size_t total_size_;
  size_t total_pos_;
  size_t vector_index_;
  size_t vector_pos_;
  size_t per_size_;
};

template <typename T>
Stream &operator<<(Stream &stream, const T &data)
{
  if (stream.total_pos_ + sizeof(T) > stream.total_size_)
    throw Error(kMemoryError, "Memory error");
  size_t data_size = sizeof(T);
  size_t current_copy_size = 0;
  size_t capacy = stream.per_size_ - stream.vector_pos_;
  while (data_size > capacy)
  {
    std::copy(reinterpret_cast<const char *>(&data + current_copy_size), reinterpret_cast<const char *>(&data + current_copy_size + capacy), stream.memory_vector_[stream.vector_index_] + stream.vector_pos_);
    stream.total_pos_ += capacy;
    data_size -= capacy;
    stream.vector_pos_ = 0;
    ++stream.vector_index_;
    capacy = stream.per_size_;
  }
  std::copy(reinterpret_cast<const char *>(&data + current_copy_size), reinterpret_cast<const char *>(&data + current_copy_size + data_size), stream.memory_vector_[stream.vector_index_] + stream.vector_pos_);
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

template <typename T>
Stream &operator>>(Stream &stream, T &data)
{
  if (stream.total_pos_ + sizeof(T) > stream.total_size_)
    throw Error(kMemoryError, "Memory error");
  size_t data_size = sizeof(T);
  size_t current_copy_size = 0;
  size_t capacy = stream.per_size_ - stream.vector_pos_;
  while (data_size > capacy)
  {
    std::copy(reinterpret_cast<const T *>(stream.memory_vector_[stream.vector_index_] + stream.vector_pos_), reinterpret_cast<const T *>(stream.memory_vector_[stream.vector_index_] + stream.vector_pos_ + capacy), &data + current_copy_size);
    stream.total_pos_ += capacy;
    data_size -= capacy;
    stream.vector_pos_ = 0;
    ++stream.vector_index_;
    capacy = stream.per_size_;
  }
  std::copy(reinterpret_cast<const T *>(stream.memory_vector_[stream.vector_index_] + stream.vector_pos_), reinterpret_cast<const T *>(stream.memory_vector_[stream.vector_index_] + stream.vector_pos_ + data_size), &data + current_copy_size);
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