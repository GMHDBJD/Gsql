#include "shell.h"

#include <iostream>

std::string Shell::getInput()
{
    std::string::size_type pos = std::string::npos;
    while ((pos = buffer.find(';')) == std::string::npos)
    {
        if (buffer.empty())
            std::cout << "Gsql> ";
        else
        {
            std::cout << "   -> ";
            buffer += '\n';
        }
        std::string str;
        std::getline(std::cin, str);
        buffer += str;
    }
    std::string temp = buffer.substr(0, pos + 1);
    buffer = buffer.substr(pos + 1);
    return temp;
}

void Shell::showResult(const Result &result)
{
    switch (result.type_)
    {
    case kShowDatabasesResult:
        std::cout << "database" << std::endl;
        for (auto &&i : result.string_vector_vector_.front())
        {
            std::cout << i << std::endl;
        }
        break;
    case kUseResult:
        std::cout << "change database" << std::endl;
        break;
    case kCreateDatabaseResult:
        std::cout << "create database" << std::endl;
        break;
    default:
        break;
    }
}

void Shell::showError(const Error &error)
{
    switch (error.getErrorCode())
    {
    case kSqlError:
        std::cout << "you have a sql error near '" << error.what() << "'" << std::endl;
        break;
    case kSyntaxTreeError:
        std::cout << "syntax tree error on '" << error.what() << "'" << std::endl;
        break;
    case kDatabaseNotExistError:
        std::cout << "database '" << error.what() << "' "
                  << "not exist" << std::endl;
        break;
    case kDatabaseExistError:
        std::cout << "database '" << error.what() << "' "
                  << "exist" << std::endl;
        break;
    case kMemoryError:
        std::cout << "memory error" << std::endl;
    default:
        break;
    }
}