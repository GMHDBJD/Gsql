#include "shell.h"
#include <iomanip>
#include <iostream>

std::string Shell::getInput()
{
    if (buffer.find_first_not_of(' ') == std::string::npos)
    {
        buffer = "";
    }
    while (buffer.empty())
    {
        char *buf = readline("Gsql> ");
        if (strlen(buf) > 0)
        {
            add_history(buf);
        }
        buffer += buf;
        free(buf);
        if (buffer.find_first_not_of(' ') == std::string::npos)
        {
            buffer = "";
        }
    }
    int state = 1;
    std::string::size_type pos = 0;
    std::string::size_type length = buffer.size();
    while (true)
    {
        if (pos == length)
        {
            buffer += '\n';
            ++length;
            char *buf = nullptr;
            if (state == 1)
                buf = readline("   -> ");
            else if (state == 2)
                buf = readline("   '>");
            else if (state == 3)
                buf = readline("   \">");
            if (strlen(buf) > 0)
            {
                add_history(buf);
                buffer += buf;
                length += strlen(buf);
            }
            free(buf);
        }
        else
        {
            if (buffer[pos] == ';' && state == 1)
                break;
            else if (buffer[pos] == '\'')
            {
                if (state == 1)
                    state = 2;
                else if (state == 2)
                    state = 1;
            }
            else if (buffer[pos] == '"')
            {
                if (state == 1)
                    state = 3;
                else if (state == 3)
                    state = 1;
            }
            else if (buffer[pos] == '\\')
                buffer[pos] = '\n';
            ++pos;
        }
    }
    std::string temp = buffer.substr(0, pos + 1);
    buffer = buffer.substr(pos + 1);
    return temp;
}

void Shell::showResult(const Result &result)
{
    switch (result.type)
    {
    case kShowDatabasesResult:
        for (auto &&i : result.string_vector_vector.front())
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
    case kShowTablesResult:
        for (auto &&i : result.string_vector_vector.front())
        {
            std::cout << i << std::endl;
        }
        break;
    case kDropDatabaseResult:
        std::cout << "drop database" << std::endl;
        break;
    case kCreateTableResult:
        std::cout << "create table" << std::endl;
        break;
    case kDropTableResult:
        std::cout << "drop table" << std::endl;
        break;
    case kExplainResult:
    {
        int width = 20;
        std::cout << std::left << std::setw(width) << std::setfill(' ') << "column_name"
                  << "default_type"
                  << "notnull"
                  << "unique"
                  << "default_value"
                  << "reference_table_name"
                  << "reference_column_name" << std::endl;
        for (auto &&i : result.string_vector_vector)
        {
            for (auto &&j : i)
            {
                std::cout << std::left << std::setw(width) << std::setfill(' ') << j;
            }
            std::cout << std::endl;
        }
        break;
    }
    case kExitResult:
        std::cout << "bye" << std::endl;
        break;
    case kSelectResult:
    {
        if (result.count == 0)
            std::cout << "empty set" << std::endl;
        else
        {
            if (result.first)
            {
                for (size_t col = 0; col < result.header.size(); col++)
                {
                    int width = result.data_type_vector[col] == 0 ? 19 : result.data_type_vector[col];
                    width = width >= result.header[col].size() ? width : result.header[col].size();
                    std::cout << "+" << std::left << std::setw(width) << std::setfill('-') << '-';
                }
                std::cout << "+" << std::endl;
                for (size_t col = 0; col < result.header.size(); col++)
                {
                    int width = result.data_type_vector[col] == 0 ? 19 : result.data_type_vector[col];
                    width = width >= result.header[col].size() ? width : result.header[col].size();
                    std::cout << "|" << std::left << std::setw(width) << std::setfill(' ') << result.header[col];
                }
                std::cout << "|" << std::endl;
                for (size_t col = 0; col < result.header.size(); col++)
                {
                    int width = result.data_type_vector[col] == 0 ? 19 : result.data_type_vector[col];
                    width = width >= result.header[col].size() ? width : result.header[col].size();
                    std::cout << "+" << std::left << std::setw(width) << std::setfill('-') << '-';
                }
                std::cout << "+" << std::endl;
            }
            for (auto &&row : result.string_vector_vector)
            {
                for (size_t col = 0; col < row.size(); col++)
                {
                    int width = result.data_type_vector[col] == 0 ? 19 : result.data_type_vector[col];
                    width = width >= result.header[col].size() ? width : result.header[col].size();
                    std::cout << "|" << std::right << std::setw(width) << std::setfill(' ') << row[col];
                }
                std::cout << "|" << std::endl;
            }
            if (result.last)
            {
                for (size_t col = 0; col < result.header.size(); col++)
                {
                    int width = result.data_type_vector[col] == 0 ? 19 : result.data_type_vector[col];
                    width = width >= result.header[col].size() ? width : result.header[col].size();
                    std::cout << "+" << std::left << std::setw(width) << std::setfill('-') << '-';
                }
                std::cout << "+" << std::endl;
                std::cout << result.count << " row in set" << std::endl;
            }
        }
        break;
    }
    case kShowIndexResult:
        for (auto &&i : result.string_vector_vector.front())
        {
            std::cout << i << std::endl;
        }
        break;
    case kCreateIndexResult:
        std::cout << "create index" << std::endl;
        break;
    case kDropIndexResult:
        std::cout << "drop index" << std::endl;
        break;
    case kInsertResult:
    case kDeleteResult:
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
        break;
    case kIncorrectDatabaseNameError:
        std::cout << "incorrect database name '" << error.what() << "' " << std::endl;
        break;
    case kIncorrectNameError:
        std::cout << "incorrect name '" << error.what() << " '" << std::endl;
        break;
    case kTableExistError:
        std::cout << "table '" << error.what() << "' "
                  << "exist" << std::endl;
        break;
    case kIncorrectTableNameError:
        std::cout << "incorrect table name '" << error.what() << "' " << std::endl;
        break;
    case kNoDatabaseSelectError:
        std::cout << "no database select" << std::endl;
        break;
    case kTableNotExistError:
        std::cout << "table '" << error.what() << "' not exist" << std::endl;
        break;
    case kColumnNotExistError:
        std::cout << "column '" << error.what() << "' not exist" << std::endl;
        break;
    case kDuplicateColumnError:
        std::cout << "duplicate column name '" << error.what() << "' " << std::endl;
        break;
    case kAddForeiginError:
        std::cout << "error when add foreign key" << std::endl;
        break;
    case kMultiplePrimaryKeyError:
        std::cout << "multiple primary key" << std::endl;
        break;
    case kInvalidDefaultValueError:
        std::cout << "invalid default value for column '" << error.what() << "' " << std::endl;
        break;
    case kOperationError:
        std::cout << "invalid operation" << std::endl;
        break;
    case kColumnCountNotMatchError:
        std::cout << "column count not match in row " << error.what() << std::endl;
        break;
    case kIncorrectValueError:
        std::cout << "incorrect value " << error.what() << std::endl;
        break;
    case kIncorrectIntegerValue:
        std::cout << "incorrect interger value " << error.what() << std::endl;
        break;
    case kNotUniqueTableError:
        std::cout << "table '" << error.what() << "' "
                  << "not unique" << std::endl;
        break;
    case kColumnAmbiguousError:
        std::cout << "column '" << error.what() << "' is ambiguous" << std::endl;
        break;
    case kUnkownColumnError:
        std::cout << "unkown column " << error.what() << std::endl;
        break;
    case kNameNoValueError:
        std::cout << "no value match column '" << error.what() << std::endl;
        break;
    case kDuplicateIndexError:
        std::cout << "duplicate index name '" << error.what() << "' " << std::endl;
        break;
    case kIndexExistError:
        std::cout << "index '" << error.what() << "' "
                  << "exist" << std::endl;
        break;
    case kIndexNotExistError:
        std::cout << "index '" << error.what() << "' not exist" << std::endl;
        break;
    case kDuplicateEntryError:
        std::cout << "duplicate entry " << error.what() << "'" << std::endl;
        break;
    case kColumnNotNullError:
        std::cout << "column '" << error.what() << "' can't be null" << std::endl;
        break;
    case kForeignkeyConstraintError:
        std::cout << "foreign key constraint error" << std::endl;
        break;
    case kUnkownTableError:
        std::cout << "unkown table '" << error.what() << "' in multiple delete" << std::endl;
        break;
    case kDataOverFlowError:
        std::cout << "data over flow" << std::endl;
        break;
    default:
        break;
    }
}

void Shell::showClock(double duration)
{
    std::cout << "(" << std::fixed << std::setprecision(2) << duration << " sec)" << std::endl;
}