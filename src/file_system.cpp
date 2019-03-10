#include "file_system.h"

FileSystem &FileSystem::getInstance()
{
    static FileSystem file_system;
    return file_system;
}