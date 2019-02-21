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