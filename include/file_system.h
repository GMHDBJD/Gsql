#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_
#include <fstream>
#include <experimental/filesystem>
#include <iostream>
#include "const.h"

namespace fs = std::experimental::filesystem;

class FileSystem
{
public:
  void read(size_t page_id, PagePtr page_ptr)
  {
    if ((page_id + 1) * kPageSize > file_size_)
    {
      write(page_id, page_ptr);
    }
    file_.seekg(page_id * kPageSize);
    file_.read(page_ptr->buffer, kPageSize);
  };
  void write(size_t page_id, PagePtr page_ptr)
  {
    file_.seekp(page_id * kPageSize);
    file_.write(page_ptr->buffer, kPageSize);
    file_.flush();
    if ((page_id + 1) * kPageSize > file_size_)
      file_size_ = (page_id + 1) * kPageSize;
  }
  void setFile(std::string filename)
  {
    file_size_ = 0;
    if (file_.is_open())
    {
      file_.close();
    }
    if (!filename.empty())
    {
      file_.open(filename, std::fstream::in | std::fstream::out | std::fstream::binary);
      file_.seekg(0, file_.end);
      file_size_ = file_.tellg();
    }
    filename_ = filename;
  }
  std::string getFilename()
  {
    return filename_;
  }
  bool exists(std::string name)
  {
    return fs::exists(name);
  }
  bool create_directory(std::string name)
  {
    return fs::create_directory(name);
  }
  decltype(fs::directory_iterator()) directory_iterator(std::string name)
  {
    return fs::directory_iterator(name);
  }
  void swap(std::fstream &other_file)
  {
    file_.swap(other_file);
  }
  bool remove(const std::string filename)
  {
    return fs::remove(filename);
  }
  void rename(const std::string &from, const std::string &to)
  {
    return fs::rename(from, to);
  }
  ~FileSystem()
  {
    if (file_)
      file_.close();
  }
  size_t size()
  {
    file_.seekg(std::fstream::end);
    return file_.tellg();
  }
  static FileSystem &getInstance();

private:
  FileSystem() {}
  std::fstream file_;
  std::string filename_;
  size_t file_size_ = 0;
};

#endif