#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_
#include <fstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

constexpr int kPageSize = 16000;
constexpr size_t kSizeOfSizeT = sizeof(size_t);
constexpr size_t kSizeOfInt = sizeof(int);
constexpr size_t kSizeOfBool = sizeof(bool);
constexpr size_t kSizeOfChar = sizeof(char);

using Page = char[kPageSize];
using PagePtr = std::shared_ptr<Page>;

class FileSystem
{
public:
  PagePtr read(size_t page_num)
  {
    file_.seekg(page_num * kPageSize);
    char *page = new Page;
    file_.read(page, kPageSize);
    PagePtr page_ptr(page);
    return page_ptr;
  };
  void write(size_t page_num, PagePtr page_ptr)
  {
    file_.seekp(page_num * kPageSize);
    file_.write(page_ptr.get(), kPageSize);
    file_.flush();
  }
  void setFile(std::string filename)
  {
    if (file_.is_open())
    {
      file_.close();
    }
    if (!filename.empty())
    {
      file_.open(filename, std::fstream::in | std::fstream::out | std::fstream::binary);
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
  ~FileSystem()
  {
    if (file_)
      file_.close();
  }
  static FileSystem& getInstance();

private:
  FileSystem(){}
  std::fstream file_;
  std::string filename_;
};


#endif