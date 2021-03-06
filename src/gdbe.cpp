#include <fstream>
#include "gdbe.h"
#include "stream.h"
#include "b_plus_tree.h"

GDBE &GDBE::getInstance()
{
    static GDBE gdbe;
    return gdbe;
}

void GDBE::exec(SyntaxTree syntax_tree)
{
    syntax_tree_ = std::move(syntax_tree);
    query_optimizer_.optimizer(&syntax_tree);
    result_.clear();
    execRoot(syntax_tree_.root_);
}

Result GDBE::getResult()
{
    if (result_.type != kSelectResult)
    {
        Result temp_result;
        std::swap(result_, temp_result);
        return temp_result;
    }
    else
    {
        result_.string_vector_vector.clear();
        if (result_.page_id == -1)
        {
            Result temp_result;
            std::swap(result_, temp_result);
            return temp_result;
        }
        if (result_.start)
        {
            Iterator iterator = BPlusTreeSelect(result_.page_id, nullptr, nullptr, false);
            result_.begin_iter = iterator.begin();
            result_.end_iter = iterator.end();
            result_.iter = result_.begin_iter;
            result_.start = false;
        }
        else
        {
            result_.first = false;
        }
        size_t count = 0;
        while (result_.iter != result_.end_iter && count < 200)
        {
            auto value_vector = toTokenResultVector(*result_.iter, result_.data_type_vector, kSizeOfSizeT);
            std::vector<std::string> temp_vector;
            for (auto &&i : value_vector)
            {
                if (i.token_type == kNull)
                    temp_vector.push_back("NULL");
                else
                    temp_vector.push_back(i.str);
            }
            result_.string_vector_vector.push_back(temp_vector);
            ++result_.iter;
            ++count;
        }
        if (result_.iter == result_.end_iter)
        {
            result_.last = true;
            Result temp_result;
            BPlusTreeRemove(result_.page_id);
            std::swap(result_, temp_result);
            return temp_result;
        }
        else
            return result_;
    }
}

void GDBE::execRoot(Node &node)
{
    switch (node.token.token_type)
    {
    case kCreate:
        execCreate(node);
        break;
    case kShow:
        execShow(node);
        break;
    case kUse:
        execUse(node);
        break;
    case kInsert:
        execInsert(node);
        break;
    case kSelect:
        execSelect(node);
        break;
    case kUpdate:
        execUpdate(node);
        break;
    case kDelete:
        execDelete(node);
        break;
    case kAlter:
        execAlter(node);
        break;
    case kDrop:
        execDrop(node);
        break;
    case kExplain:
        execExplain(node);
        break;
    case kExit:
        result_.type = kExitResult;
        break;
    default:
        break;
    }
}

void GDBE::execCreate(const Node &create_node)
{
    const Node &next_node = create_node.children.front();
    switch (next_node.token.token_type)
    {
    case kDatabase:
        execCreateDatabase(next_node);
        break;
    case kTable:
        execCreateTable(next_node);
        break;
    case kIndex:
        execCreateIndex(next_node);
        break;
    default:
        break;
    }
}

void GDBE::execCreateDatabase(const Node &database_node)
{
    const Node &string_node = database_node.children.front();
    if (!file_system_.exists(kDatabaseDir))
        file_system_.create_directory(kDatabaseDir);
    if (file_system_.exists(kDatabaseDir + string_node.token.str))
        throw Error(kDatabaseExistError, string_node.token.str);
    DatabaseSchema new_database_schema;
    new_database_schema.page_vector.push_back(0);
    std::vector<PagePtr> page_ptr_vector;
    PagePtr page_ptr(new Page);
    page_ptr_vector.push_back(page_ptr);
    Stream stream(page_ptr_vector);
    stream << new_database_schema;
    std::fstream file(string_node.token.str, std::fstream::out);
    file_system_.swap(file);
    file_system_.write(0, page_ptr_vector[0]);
    file_system_.swap(file);
    file.close();
    file_system_.rename(string_node.token.str, kDatabaseDir + string_node.token.str);
    result_.type = kCreateDatabaseResult;
}

void GDBE::execShow(const Node &show_node)
{
    const Node &next_node = show_node.children.front();
    switch (next_node.token.token_type)
    {
    case kDatabases:
        execShowDatabases(next_node);
        break;
    case kTables:
        execShowTables(next_node);
        break;
    case kIndex:
        execShowIndex(next_node);
        break;
    default:
        throw Error(kSyntaxTreeError, next_node.token.str);
    }
}

void GDBE::execShowDatabases(const Node &databases_node)
{
    if (!file_system_.exists(kDatabaseDir))
    {
        result_.type = kShowDatabasesResult;
        return;
    }
    std::vector<std::string> string_vector;
    for (auto &entry : fs::directory_iterator(kDatabaseDir))
    {
        string_vector.push_back(entry.path().filename().string());
    }
    result_.type = kShowDatabasesResult;
    result_.string_vector_vector.push_back(std::move(string_vector));
}

void GDBE::execUse(const Node &use_node)
{
    const Node &string_node = use_node.children.front().children.front();
    if (string_node.token.str != database_name_)
    {
        if (!file_system_.exists(kDatabaseDir + string_node.token.str))
            throw Error(kDatabaseNotExistError, string_node.token.str);
        file_system_.setFile(kDatabaseDir + string_node.token.str);
        buffer_pool_.clear();
        std::vector<PagePtr> page_ptr_vector{buffer_pool_.getPage(0)};
        Stream stream(page_ptr_vector);
        DatabaseSchema new_database_schema;
        stream >> new_database_schema.page_vector;
        page_ptr_vector.clear();
        for (auto &&i : new_database_schema.page_vector)
        {
            page_ptr_vector.push_back(buffer_pool_.getPage(i));
        }
        stream.setBuffer(page_ptr_vector);
        stream >> new_database_schema;
        database_schema_.swap(new_database_schema);
        database_name_ = string_node.token.str;
    }
    result_.type = kUseResult;
}

void GDBE::execDrop(const Node &drop_node)
{
    const Node &next_node = drop_node.children.front();
    switch (next_node.token.token_type)
    {
    case kDatabase:
        execDropDatabase(next_node);
        break;
    case kTable:
        execDropTable(next_node);
        break;
    case kIndex:
        execDropIndex(next_node);
        break;
    default:
        throw Error(kSyntaxTreeError, next_node.token.str);
    }
}

void GDBE::execDropDatabase(const Node &database_node)
{
    const Node &string_node = database_node.children.front().children.front();
    if (!file_system_.exists(kDatabaseDir + string_node.token.str))
        throw Error(kDatabaseNotExistError, string_node.token.str);
    if (file_system_.getFilename() == kDatabaseDir + string_node.token.str)
        file_system_.setFile("");
    if (database_name_ == string_node.token.str)
    {
        database_schema_.clear();
        database_name_ = "";
    }
    file_system_.remove(kDatabaseDir + string_node.token.str);
    result_.type = kDropDatabaseResult;
}

void GDBE::execCreateTable(const Node &table_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    std::string table_name = getTableName(table_node.children.front(), database_name_);
    if (database_schema_.table_schema_map.find(table_name) != database_schema_.table_schema_map.end())
        throw Error(kTableExistError, table_name);
    TableSchema new_table_schema;
    const Node &columns_node = table_node.children.back();
    for (const auto &node : columns_node.children)
    {
        if (node.token.token_type == kColumn)
        {
            ColumnSchema new_column_schema;
            std::string column_name = getColumnName(node.children.front(), database_name_, table_name);
            switch (node.children[1].token.token_type)
            {
            case kInt:
                new_column_schema.data_type = 0;
                break;
            case kChar:
            {
                if (node.children[1].children.empty())
                    new_column_schema.data_type = 1;
                else
                    new_column_schema.data_type = node.children[1].children.front().token.num;
                break;
            }
            default:
                throw Error(kSyntaxTreeError, node.children[1].token.str);
            }
            for (size_t i = 2; i < node.children.size(); i++)
            {
                switch (node.children[i].token.token_type)
                {
                case kNotNull:
                {
                    new_column_schema.not_null = true;
                    if (new_column_schema.null_default)
                    {
                        new_column_schema.null_default = false;
                        if (new_column_schema.data_type == 0)
                            new_column_schema.default_value = "0";
                        else
                            new_column_schema.default_value = "";
                    }
                    break;
                }
                case kDefault:
                {
                    new_column_schema.null_default = false;
                    Node value_node = node.children.back().children.front();
                    try
                    {
                        if (value_node.token.token_type == kString && new_column_schema.data_type == 0)
                            convertInt(value_node.token);
                        else if ((value_node.token.token_type == kNum && new_column_schema.data_type != 0))
                            convertString(value_node.token);
                    }
                    catch (const Error &e)
                    {
                        throw Error(kInvalidDefaultValueError, column_name);
                    }
                    new_column_schema.default_value = value_node.token.str;
                    break;
                }
                case kUnique:
                {
                    new_column_schema.unique = true;
                    if (new_column_schema.index_schema.root_page_id == -1)
                    {
                        size_t index_size = new_column_schema.data_type == 0 ? kSizeOfLong + kSizeOfBool : new_column_schema.data_type + kSizeOfBool;
                        PageSchema index_page_schema(true, 0, -1, -1, index_size + kSizeOfSizeT, index_size, kSizeOfSizeT, !new_column_schema.data_type);
                        IndexSchema index_schema;
                        index_schema.column_name = column_name;
                        index_schema.root_page_id = createNewPage(index_page_schema);
                        new_column_schema.index_schema = index_schema;
                    }
                    break;
                }
                default:
                    throw Error(kSyntaxTreeError, node.children.back().token.str);
                }
            }
            auto rc = new_table_schema.column_schema_map.insert({column_name, new_column_schema});
            if (!rc.second)
                throw Error(kDuplicateColumnError, column_name);
            if (node.children.size() == 3)
                new_column_schema.not_null = true;
            new_table_schema.column_order_vector.push_back(column_name);
        }
    }
    for (const auto &node : columns_node.children)
    {
        if (node.token.token_type == kForeign)
        {
            const Node &names_node = node.children.front();
            std::string column_name = getColumnName(node.children[0], database_name_, table_name);
            std::string reference_table_name = getTableName(node.children[1], database_name_);
            std::string reference_column_name = getColumnName(node.children[2], database_name_, reference_table_name);
            auto column_iter = new_table_schema.column_schema_map.find(column_name);
            if (column_iter == new_table_schema.column_schema_map.end())
                throw Error(kAddForeiginError, "");
            auto reference_table_iter = database_schema_.table_schema_map.find(reference_table_name);
            if (reference_table_iter == database_schema_.table_schema_map.end())
                throw Error(kAddForeiginError, "");
            auto reference_column_iter = reference_table_iter->second.column_schema_map.find(reference_column_name);
            if (reference_column_iter == reference_table_iter->second.column_schema_map.end())
                throw Error(kAddForeiginError, "");
            if (reference_column_iter->second.data_type != column_iter->second.data_type || !reference_column_iter->second.unique)
                throw Error(kAddForeiginError, "");
            if (!column_iter->second.reference_table_name.empty() && column_iter->second.reference_table_name != reference_table_name)
                throw Error(kAddForeiginError, "");
            if (!column_iter->second.reference_column_name.empty() && column_iter->second.reference_column_name != reference_column_name)
                throw Error(kAddForeiginError, "");
        }
        else if (node.token.token_type == kPrimary)
        {
            if (!new_table_schema.primary_set.empty())
                throw Error(kMultiplePrimaryKeyError, "");
            const Node &names_node = node.children.front();
            for (const auto &name_node : names_node.children)
            {
                std::string column_name = getColumnName(name_node, database_name_, table_name);
                if (new_table_schema.column_schema_map.find(column_name) == new_table_schema.column_schema_map.end())
                    throw Error(kColumnNotExistError, column_name);
                auto rc = new_table_schema.primary_set.insert(column_name);
                if (new_table_schema.primary_set.size() > 1)
                    throw Error(kSyntaxTreeError, name_node.token.str);
            }
        }
    }

    for (const auto &node : columns_node.children)
    {
        if (node.token.token_type == kForeign)
        {
            const Node &names_node = node.children.front();
            std::string column_name = getColumnName(node.children[0], database_name_, table_name);
            std::string reference_table_name = getTableName(node.children[1], database_name_);
            std::string reference_column_name = getColumnName(node.children[2], database_name_, reference_table_name);
            auto column_iter = new_table_schema.column_schema_map.find(column_name);
            auto reference_table_iter = database_schema_.table_schema_map.find(reference_table_name);
            auto reference_column_iter = reference_table_iter->second.column_schema_map.find(reference_column_name);
            column_iter->second.reference_column_name = reference_column_name;
            column_iter->second.reference_table_name = reference_table_name;
            auto &column_schema = new_table_schema.column_schema_map[column_name];
            if (column_schema.index_schema.root_page_id == -1)
            {
                size_t index_size = column_schema.data_type == 0 ? kSizeOfLong + kSizeOfBool : column_schema.data_type + kSizeOfBool;
                PageSchema index_page_schema(true, 0, -1, -1, index_size + kSizeOfSizeT, index_size, kSizeOfSizeT, !column_schema.data_type);
                IndexSchema index_schema;
                index_schema.column_name = column_name;
                index_schema.root_page_id = createNewPage(index_page_schema);
                column_schema.index_schema = index_schema;
            }
            database_schema_.table_schema_map[reference_table_name].column_schema_map[reference_column_name].be_reference_set.insert({table_name, column_name});
        }
        else if (node.token.token_type == kPrimary)
        {
            const Node &names_node = node.children.front();
            for (const auto &name_node : names_node.children)
            {
                std::string column_name = getColumnName(name_node, database_name_, table_name);
                auto rc = new_table_schema.primary_set.insert(column_name);
                auto &column_schema = new_table_schema.column_schema_map[column_name];
                column_schema.unique = true;
                column_schema.not_null = true;
                if (column_schema.null_default)
                {
                    column_schema.null_default = false;
                    if (column_schema.data_type == 0)
                        column_schema.default_value = "0";
                    else
                        column_schema.default_value = "";
                }
                if (column_schema.index_schema.root_page_id == -1)
                {
                    size_t index_size = column_schema.data_type == 0 ? kSizeOfLong + kSizeOfBool : column_schema.data_type + kSizeOfBool;
                    PageSchema index_page_schema(true, 0, -1, -1, index_size + kSizeOfSizeT, index_size, kSizeOfSizeT, !column_schema.data_type);
                    IndexSchema index_schema;
                    index_schema.column_name = column_name;
                    index_schema.root_page_id = createNewPage(index_page_schema);
                    column_schema.index_schema = index_schema;
                }
            }
        }
    }
    new_table_schema.max_id = 0;
    size_t value_size = getValueSize(new_table_schema.column_schema_map);
    if (dataOverFlow(kSizeOfSizeT + kSizeOfBool, value_size))
        throw Error(kDataOverFlowError, "");
    PageSchema new_page_schema(true, 0, -1, -1, kSizeOfSizeT + kSizeOfBool, kSizeOfSizeT + kSizeOfBool, value_size, true);

    new_table_schema.root_page_id = createNewPage(new_page_schema);
    database_schema_.table_schema_map[table_name] = new_table_schema;
    updateDatabaseSchema();
    result_.type = kCreateTableResult;
}

void GDBE::execShowTables(const Node &tables_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    std::vector<std::string> string_vector;
    for (const auto &i : database_schema_.table_schema_map)
    {
        string_vector.push_back(i.first);
    }
    result_.type = kShowTablesResult;
    result_.string_vector_vector.push_back(std::move(string_vector));
}

void GDBE::execDropTable(const Node &table_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    const Node &name_node = table_node.children.front();
    std::string table_name = getTableName(name_node, database_name_);
    auto table_iter = database_schema_.table_schema_map.find(table_name);
    if (table_iter == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    for (auto &&i : table_iter->second.column_schema_map)
    {
        if (!i.second.be_reference_set.empty())
            throw Error(kForeignkeyConstraintError, "");
    }
    for (auto &&i : table_iter->second.index_schema_map)
    {
        for (auto &&j : i.second)
        {
            BPlusTreeRemove(j.second.root_page_id);
        }
    }
    for (auto &&i : table_iter->second.column_schema_map)
    {
        if (i.second.index_schema.root_page_id != -1)
            BPlusTreeRemove(i.second.index_schema.root_page_id);
    }
    for (auto &&i : table_iter->second.column_schema_map)
    {
        if (!i.second.reference_table_name.empty())
        {
            std::string reference_table_name = i.second.reference_table_name;
            std::string reference_column_name = i.second.reference_column_name;
            database_schema_.table_schema_map[reference_table_name].column_schema_map[reference_column_name].be_reference_set.erase({table_name, i.first});
        }
    }
    size_t page_id = table_iter->second.root_page_id;
    BPlusTreeRemove(page_id);
    database_schema_.table_schema_map.erase(table_iter->first);
    updateDatabaseSchema();
    result_.type = kDropTableResult;
}

void GDBE::execInsert(Node &insert_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    const Node &name_node = insert_node.children.front();
    std::string table_name = getTableName(name_node, database_name_);
    auto table_schema_iter = database_schema_.table_schema_map.find(table_name);
    if (table_schema_iter == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    std::unordered_map<std::string, size_t> name_pos_map;
    std::unordered_set<std::string> table_name_set;
    table_name_set.insert(table_name);
    if (insert_node.children.size() == 3)
    {
        int pos = 0;
        for (auto &&name_node : insert_node.children[1].children)
        {
            std::string column_name = getColumnName(name_node, database_name_, table_name);
            if (database_schema_.table_schema_map[table_name].column_schema_map.find(column_name) == database_schema_.table_schema_map[table_name].column_schema_map.end())
                throw Error(kColumnNotExistError, column_name);
            if (name_pos_map.find(column_name) != name_pos_map.end())
                throw Error(kDuplicateColumnError, column_name);
            name_pos_map[column_name] = pos++;
        }
    }
    else
    {
        int pos = 0;
        for (auto &&name : database_schema_.table_schema_map[table_name].column_order_vector)
        {
            name_pos_map[name] = pos++;
        }
    }
    int row = 0;
    for (auto &exprs_node : insert_node.children.back().children)
    {
        ++row;
        if (exprs_node.children.size() != name_pos_map.size())
            throw Error(kColumnCountNotMatchError, std::to_string(row));
        std::vector<Token> values;
        std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column_value_map;
        std::vector<size_t> values_size;
        size_t id = table_schema_iter->second.max_id++;
        for (auto &expr_node : exprs_node.children)
        {
            check(expr_node.children.front(), table_name_set, database_name_, database_schema_);
        }

        for (auto &&column_name : database_schema_.table_schema_map[table_name].column_order_vector)
        {
            const ColumnSchema &column_schema = database_schema_.table_schema_map[table_name].column_schema_map[column_name];
            if (column_schema.null_default)
                table_column_value_map[table_name][column_name] = Token(kNull);
            else if (column_schema.data_type == 0)
                table_column_value_map[table_name][column_name] = Token(kNum, column_schema.default_value, std::atol(column_schema.default_value.c_str()));
            else
                table_column_value_map[table_name][column_name] = Token(kString, column_schema.default_value);
        }
        for (auto &&column_name : database_schema_.table_schema_map[table_name].column_order_vector)
        {
            const ColumnSchema &column_schema = database_schema_.table_schema_map[table_name].column_schema_map[column_name];
            Node result_node;
            if (name_pos_map.find(column_name) == name_pos_map.end())
                result_node = table_column_value_map[table_name][column_name];
            else
            {
                size_t pos = name_pos_map[column_name];
                result_node = eval(exprs_node.children[pos].children.front(), table_column_value_map, false);
            }
            if (column_schema.data_type == 0 && result_node.token.token_type == kString)
            {
                try
                {
                    convertInt(result_node.token);
                }
                catch (const Error &e)
                {
                    throw Error(kIncorrectIntegerValue, "\"" + result_node.token.str + "\"" + " for column " + column_name + " at row " + std::to_string(row));
                }
            }
            else if (column_schema.data_type > 0 && result_node.token.token_type == kNum)
            {
                convertString(result_node.token);
            }
            values.push_back(result_node.token);
            values_size.push_back(column_schema.data_type);
            table_column_value_map[table_name][column_name] = result_node.token;
        }
        for (auto &&i : table_column_value_map[table_name])
        {
            const auto &column_schema = table_schema_iter->second.column_schema_map[i.first];
            size_t size = column_schema.data_type == 0 ? kSizeOfLong : column_schema.data_type;
            char *key = new char[size + kSizeOfBool + kSizeOfSizeT];
            serilization({i.second}, {size}, key);
            if (column_schema.unique)
            {
                if (BPlusTreeSearch(column_schema.index_schema.root_page_id, key, true))
                {
                    delete[] key;
                    throw Error(kDuplicateEntryError, "'" + i.second.str + "' for key '" + i.first);
                }
            }
            if (column_schema.not_null && i.second.token_type == kNull)
            {
                delete[] key;
                throw Error(kColumnNotNullError, i.first);
            }
            if (!column_schema.reference_column_name.empty())
            {
                TableSchema reference_table_schema = database_schema_.table_schema_map[column_schema.reference_table_name];
                ColumnSchema reference_column_schema = reference_table_schema.column_schema_map[column_schema.reference_column_name];
                if (!BPlusTreeSearch(reference_column_schema.index_schema.root_page_id, key, true))
                {
                    delete[] key;
                    throw Error(kForeignkeyConstraintError, "");
                }
            }
            delete[] key;
        }
        char *key_ptr = new char[kSizeOfSizeT + kSizeOfBool];
        bool null = false;
        std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, key_ptr);
        std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, key_ptr + kSizeOfBool);
        size_t size = getValueSize(table_schema_iter->second.column_schema_map);
        char *values_ptr = new char[size];
        serilization(values, values_size, values_ptr);
        size_t root_page_id = table_schema_iter->second.root_page_id;
        BPlusTreeInsert(root_page_id, key_ptr, values_ptr, false, &root_page_id);
        database_schema_.table_schema_map[table_name].root_page_id = root_page_id;
        for (auto &&i : table_column_value_map[table_name])
        {
            auto &column_schema = table_schema_iter->second.column_schema_map[i.first];
            if (column_schema.index_schema.root_page_id != -1)
            {
                auto &index_schema = column_schema.index_schema;
                size_t size = column_schema.data_type == 0 ? kSizeOfLong : column_schema.data_type;
                char *key = new char[size + kSizeOfBool + kSizeOfSizeT];
                char *value = new char[kSizeOfSizeT];
                size_t index_page_id = database_schema_.table_schema_map[table_name].column_schema_map[i.first].index_schema.root_page_id;
                serilization({i.second}, {size}, key);
                std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, key + size + kSizeOfBool);
                std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, value);
                BPlusTreeInsert(index_schema.root_page_id, key, value, false, &index_page_id);
                database_schema_.table_schema_map[table_name].column_schema_map[i.first].index_schema.root_page_id = index_page_id;
                delete[] key;
                delete[] value;
            }
        }
        for (auto &&i : table_column_value_map[table_name])
        {
            if (table_schema_iter->second.index_schema_map.find({i.first}) != table_schema_iter->second.index_schema_map.end())
            {
                for (auto &&index_pair : table_schema_iter->second.index_schema_map[{i.first}])
                {
                    IndexSchema &index_schema = index_pair.second;
                    if (index_schema.root_page_id != -1)
                    {
                        auto &column_schema = table_schema_iter->second.column_schema_map[index_schema.column_name];
                        size_t size = column_schema.data_type == 0 ? kSizeOfLong : column_schema.data_type;
                        char *key = new char[size + kSizeOfBool + kSizeOfSizeT];
                        char *value = new char[kSizeOfSizeT];
                        size_t index_page_id = database_schema_.table_schema_map[table_name].index_schema_map[{i.first}][index_pair.first].root_page_id;
                        serilization({i.second}, {size}, key);
                        std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, key + size + kSizeOfBool);
                        std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, value);
                        BPlusTreeInsert(index_page_id, key, value, false, &index_page_id);
                        database_schema_.table_schema_map[table_name].index_schema_map[{i.first}][index_pair.first].root_page_id = index_page_id;
                        delete[] key;
                        delete[] value;
                    }
                }
            }
        }
        updateDatabaseSchema();
        delete[] key_ptr;
        delete[] values_ptr;
    }
}

void GDBE::execExplain(const Node &explain_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    const Node &name_node = explain_node.children.front();
    std::string table_name = getTableName(name_node, database_name_);
    auto table_iter = database_schema_.table_schema_map.find(table_name);
    if (table_iter == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    TableSchema table_schema = table_iter->second;
    for (size_t i = 0; i < table_schema.column_order_vector.size(); i++)
    {
        std::vector<std::string> string_vector;
        string_vector.push_back(table_schema.column_order_vector[i]);
        const auto &column_schema = table_schema.column_schema_map[table_schema.column_order_vector[i]];
        string_vector.push_back(std::to_string(column_schema.data_type));
        string_vector.push_back(std::to_string(column_schema.not_null));
        string_vector.push_back(std::to_string(column_schema.unique));
        string_vector.push_back(column_schema.default_value);
        string_vector.push_back(column_schema.reference_table_name);
        string_vector.push_back(column_schema.reference_column_name);
        result_.string_vector_vector.push_back(std::move(string_vector));
    }
    result_.type = kExplainResult;
}

void GDBE::execSelect(Node &select_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");

    std::unordered_set<std::string> select_table_name_set;
    std::vector<Node> select_expr_node_vector;
    std::vector<Node> condition_node_vector;

    const Node &names_node = select_node.children[1];
    for (auto &&i : names_node.children)
    {
        std::string table_name = getTableName(i, database_name_);
        const auto &table_schema_iter = database_schema_.table_schema_map.find(table_name);
        if (table_schema_iter == database_schema_.table_schema_map.end())
            throw Error(kTableNotExistError, table_name);
        if (select_table_name_set.find(table_name) != select_table_name_set.end())
            throw Error(kNotUniqueTableError, table_name);
        select_table_name_set.insert(table_name);
    }
    if (select_node.children.size() > 2 && select_node.children[2].token.token_type == kJoins)
    {
        for (auto &join_node : select_node.children[2].children)
        {
            std::string table_name = getTableName(join_node.children.front(), database_name_);
            const auto &table_schema_iter = database_schema_.table_schema_map.find(table_name);
            if (table_schema_iter == database_schema_.table_schema_map.end())
                throw Error(kTableNotExistError, table_name);
            if (select_table_name_set.find(table_name) != select_table_name_set.end())
                throw Error(kNotUniqueTableError, table_name);
            select_table_name_set.insert(table_name);
            if (join_node.children.back().token.token_type == kExpr)
            {
                check(join_node.children.back().children.front(), select_table_name_set, database_name_, database_schema_);
                condition_node_vector.push_back(join_node.children.back().children.front());
            }
        }
    }
    Node &columns_node = select_node.children.front();
    if (columns_node.children.front().token.token_type == kMultiply)
    {
        for (auto &&i : select_table_name_set)
        {
            for (auto &&j : database_schema_.table_schema_map[i].column_order_vector)
            {
                Node name_node{Token(kName, "NAME")};
                name_node.children.emplace_back(Token(kString, i));
                name_node.children.emplace_back(Token(kString, j));
                result_.header.push_back(i + "." + j);
                select_expr_node_vector.push_back(name_node);
            }
        }
    }
    if (columns_node.children.back().token.token_type == kExprs)
    {
        for (auto &expr_node : columns_node.children.back().children)
        {
            result_.header.push_back(expr_node.children.front().token.str);
            check(expr_node.children.front(), select_table_name_set, database_name_, database_schema_);
            select_expr_node_vector.push_back(expr_node.children.front());
        }
    }
    size_t where_pos = select_node.children.size() - 1;
    size_t limit = -1;
    if (select_node.children.back().token.token_type == kLimit)
    {
        limit = select_node.children.back().children.front().token.num;
        --where_pos;
    }
    if (select_node.children[where_pos].token.token_type == kWhere)
    {
        Node &expr_node = select_node.children[where_pos].children.front().children.front();
        check(expr_node, select_table_name_set, database_name_, database_schema_);
        condition_node_vector.push_back(expr_node);
    }
    std::vector<Node> condition_vector = splitConditionVector(condition_node_vector);
    for (auto &i : condition_vector)
    {
        i = eval(i, {}, true);
    }
    std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> table_condition_map;
    std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> table_index_condition_map;
    bool rc = partitionConditionForTable(condition_vector, &table_condition_map);
    if (!rc)
        result_.type = kNoneResult;
    else
    {
        for (auto &i : table_condition_map)
            if (i.first.size() == 1)
                table_index_condition_map[*i.first.begin()] = getCondition(i.second, &rc);
        if (!rc)
        {
            result_.type = kNoneResult;
            return;
        }
        selectRecursive(table_index_condition_map, table_condition_map, select_table_name_set, select_expr_node_vector, limit);
        result_.type = kSelectResult;
    }
}

void GDBE::selectRecursive(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition_map, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, const std::unordered_set<std::string> &select_table_name_set, const std::vector<Node> &select_expr_vector, size_t limit)
{
    selectRecursiveAux(table_index_condition_map, table_condition_map, select_table_name_set, {}, {}, select_expr_vector, limit);
}

void GDBE::selectRecursiveAux(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition_map, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, std::unordered_set<std::string> remain_table_name_set, std::unordered_set<std::string> already_table_name_set, std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column_map, const std::vector<Node> &select_expr_vector, size_t limit)
{
    if (result_.count == limit)
        return;
    if (!remain_table_name_set.empty())
    {
        std::string table_name = *remain_table_name_set.begin();
        remain_table_name_set.erase(remain_table_name_set.begin());
        already_table_name_set.insert(table_name);
        const TableSchema &table_schema = database_schema_.table_schema_map[table_name];
        const auto &index_condition_map_iter = table_index_condition_map.find(table_name);
        if (index_condition_map_iter != table_index_condition_map.end() && index_condition_map_iter->second.first.root_page_id != -1)
        {
            std::pair<IndexSchema, std::pair<Token, Token>> index_condition = index_condition_map_iter->second;
            std::pair<Token, Token> condition = index_condition.second;
            const IndexSchema &index_schema = index_condition.first;
            std::string column_name = index_schema.column_name;
            const ColumnSchema &column_schema = database_schema_.table_schema_map[table_name].column_schema_map[column_name];
            int data_type = column_schema.data_type;
            size_t size = data_type == 0 ? kSizeOfLong : data_type;
            Token token;
            if (data_type == 0)
            {
                convertInt(condition.first);
                convertInt(condition.second);
            }
            else
            {
                convertString(condition.first);
                convertString(condition.second);
            }
            char *begin_key = nullptr, *end_key = nullptr;
            if (condition.first)
            {
                begin_key = new char[size + kSizeOfBool];
                serilization({condition.first}, {size}, begin_key);
            }
            if (condition.second)
            {
                end_key = new char[size + kSizeOfBool];
                serilization({condition.second}, {size}, end_key);
            }
            size_t index_page_id = index_schema.root_page_id;
            PageSchema temp_page_schema(true, 0, -1, -1, kSizeOfSizeT + kSizeOfBool, kSizeOfSizeT + kSizeOfBool, 0, true);
            size_t temp_page_id = createNewPage(temp_page_schema);
            char *temp_key = new char[kSizeOfSizeT + kSizeOfBool];
            bool null = false;
            for (auto &&i : BPlusTreeSelect(index_page_id, begin_key, end_key, true))
            {
                size_t id = *reinterpret_cast<const size_t *>(i + size + kSizeOfBool + kSizeOfSizeT);
                std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, temp_key);
                std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, temp_key + kSizeOfBool);
                BPlusTreeInsert(temp_page_id, temp_key, nullptr, true, &temp_page_id);
            }
            delete[] temp_key;
            for (auto &&id_iter : BPlusTreeSelect(temp_page_id, nullptr, nullptr, false))
            {
                size_t id = *reinterpret_cast<const size_t *>(id_iter + kSizeOfBool);
                bool is_true = true;
                char *iter = BPlusTreeSearch(table_schema.root_page_id, id_iter, false);
                std::unordered_map<std::string, Token> column_token_map = toTokenMap(iter, table_schema, size, &id);
                table_column_map[table_name] = column_token_map;
                for (const auto &i : table_condition_map)
                {
                    if (i.first.find(table_name) == i.first.end())
                        continue;
                    else
                    {
                        bool flag = true;
                        for (const auto &j : i.first)
                        {
                            if (already_table_name_set.find(j) == already_table_name_set.end())
                            {
                                flag = false;
                                break;
                            }
                        }
                        if (flag)
                        {
                            for (const auto &r : i.second)
                            {
                                Node result_node = eval(r, table_column_map, false);
                                if (result_node.token.token_type != kNum || !result_node.token.num)
                                {
                                    is_true = false;
                                    break;
                                }
                            }
                        }
                    }
                    if (!is_true)
                        break;
                }
                if (is_true)
                    selectRecursiveAux(table_index_condition_map, table_condition_map, remain_table_name_set, already_table_name_set, table_column_map, select_expr_vector, limit);
                if (result_.count == limit)
                {
                    BPlusTreeRemove(temp_page_id);
                    if (begin_key)
                        delete[] begin_key;
                    if (end_key)
                        delete[] end_key;
                    return;
                }
            }
            BPlusTreeRemove(temp_page_id);
            if (begin_key)
                delete[] begin_key;
            if (end_key)
                delete[] end_key;
        }
        else
        {
            for (auto &&iter : BPlusTreeSelect(table_schema.root_page_id, nullptr, nullptr, false))
            {
                bool is_true = true;
                size_t id = -1;
                std::unordered_map<std::string, Token> column_token_map = toTokenMap(iter, table_schema, kSizeOfSizeT, &id);
                table_column_map[table_name] = column_token_map;

                for (const auto &i : table_condition_map)
                {
                    if (i.first.find(table_name) == i.first.end())
                        continue;
                    else
                    {
                        bool flag = true;
                        for (const auto &j : i.first)
                        {
                            if (already_table_name_set.find(j) == already_table_name_set.end())
                            {
                                flag = false;
                                break;
                            }
                        }
                        if (flag)
                        {
                            for (const auto &r : i.second)
                            {
                                Node result_node = eval(r, table_column_map, false);
                                if (result_node.token.token_type != kNum || !result_node.token.num)
                                {
                                    is_true = false;
                                    break;
                                }
                            }
                        }
                    }
                    if (!is_true)
                        break;
                }
                if (is_true)
                    selectRecursiveAux(table_index_condition_map, table_condition_map, remain_table_name_set, already_table_name_set, table_column_map, select_expr_vector, limit);
                if (result_.count == limit)
                {
                    return;
                }
            }
        }
    }
    else
    {
        std::vector<Token> value;
        ++result_.count;
        if (result_.page_id == -1)
        {
            for (const auto &i : select_expr_vector)
            {
                Node node = eval(i, table_column_map, false);
                value.push_back(node.token);
                int data_type = getExprDataType(i);
                size_t temp = data_type == 0 ? kSizeOfLong : data_type;
                result_.value_size_vector.push_back(temp);
                result_.data_type_vector.push_back(data_type);
                result_.total_size += kSizeOfBool + temp;
            }
            PageSchema result_page_schema(true, 0, -1, -1, kSizeOfSizeT + kSizeOfBool, kSizeOfSizeT + kSizeOfBool, result_.total_size, true);
            result_.page_id = createNewPage(result_page_schema);
        }
        else
        {
            for (const auto &i : select_expr_vector)
            {
                Node node = eval(i, table_column_map, false);
                value.push_back(node.token);
            }
        }
        char *key_ptr = new char[kSizeOfSizeT + kSizeOfBool];
        bool null = false;
        std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, key_ptr);
        std::copy(reinterpret_cast<const char *>(&result_.count), reinterpret_cast<const char *>(&result_.count) + kSizeOfSizeT, key_ptr + kSizeOfBool);
        char *values_ptr = new char[result_.total_size];
        serilization(value, result_.value_size_vector, values_ptr);
        size_t root_page_id = -1;
        BPlusTreeInsert(result_.page_id, key_ptr, values_ptr, false, &root_page_id);
        if (root_page_id != -1)
            result_.page_id = root_page_id;
        delete[] key_ptr;
        delete[] values_ptr;
    }
}

void GDBE::execDelete(Node &delete_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");

    std::unordered_set<std::string> delete_table_name_set;
    std::unordered_set<std::string> select_table_name_set;
    std::vector<Node> condition_node_vector;

    if (delete_node.children.front().token.token_type == kName)
    {
        std::string table_name = getTableName(delete_node.children.front(), database_name_);
        if (database_schema_.table_schema_map.find(table_name) == database_schema_.table_schema_map.end())
            throw Error(kTableNotExistError, table_name);
        delete_table_name_set.insert(table_name);
        select_table_name_set.insert(table_name);
    }
    else
    {
        for (auto &&name_node : delete_node.children.front().children)
        {
            std::string table_name = getTableName(name_node, database_name_);
            if (delete_table_name_set.find(table_name) != delete_table_name_set.end())
                throw Error(kNotUniqueTableError, table_name);
            delete_table_name_set.insert(table_name);
        }

        for (auto &&name_node : delete_node.children[1].children)
        {
            std::string table_name = getTableName(name_node, database_name_);
            const auto &table_schema_iter = database_schema_.table_schema_map.find(table_name);
            if (table_schema_iter == database_schema_.table_schema_map.end())
                throw Error(kTableNotExistError, table_name);
            if (select_table_name_set.find(table_name) != select_table_name_set.end())
                throw Error(kNotUniqueTableError, table_name);
            select_table_name_set.insert(table_name);
        }
        if (delete_node.children.size() >= 3 && delete_node.children[2].token.token_type == kJoins)
        {
            for (auto &join_node : delete_node.children[2].children)
            {
                std::string table_name = getTableName(join_node.children.front(), database_name_);
                const auto &table_schema_iter = database_schema_.table_schema_map.find(table_name);
                if (table_schema_iter == database_schema_.table_schema_map.end())
                    throw Error(kTableNotExistError, table_name);
                if (select_table_name_set.find(table_name) != select_table_name_set.end())
                    throw Error(kNotUniqueTableError, table_name);
                select_table_name_set.insert(table_name);
                if (join_node.children.back().token.token_type == kExpr)
                {
                    check(join_node.children.back().children.front(), select_table_name_set, database_name_, database_schema_);
                    condition_node_vector.push_back(join_node.children.back().children.front());
                }
            }
        }
        for (auto &&table_name : delete_table_name_set)
        {
            if (select_table_name_set.find(table_name) == select_table_name_set.end())
                throw Error(kUnkownTableError, table_name);
        }
    }
    if (delete_node.children[delete_node.children.size() - 1].token.token_type == kWhere)
    {
        Node &expr_node = delete_node.children[delete_node.children.size() - 1].children.front().children.front();
        check(expr_node, select_table_name_set, database_name_, database_schema_);
        condition_node_vector.push_back(expr_node);
    }
    std::vector<Node> condition_vector = splitConditionVector(condition_node_vector);
    for (auto &i : condition_vector)
    {
        i = eval(i, {}, true);
    }
    std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> table_condition_map;
    std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> table_index_condition_map;
    bool rc = partitionConditionForTable(condition_vector, &table_condition_map);
    if (!rc)
        result_.type = kNoneResult;
    else
    {
        for (auto &i : table_condition_map)
            if (i.first.size() == 1)
                table_index_condition_map[*i.first.begin()] = getCondition(i.second, &rc);
        if (!rc)
        {
            result_.type = kNoneResult;
            return;
        }
        std::unordered_map<std::string, size_t> table_id_page_id_map;
        for (auto &&i : delete_table_name_set)
        {
            table_id_page_id_map[i] = -1;
        }
        bool null = false;
        deleteRecursive(table_index_condition_map, table_condition_map, select_table_name_set, delete_table_name_set, table_id_page_id_map);
        for (auto &&i : table_id_page_id_map)
        {
            std::string table_name = i.first;
            size_t id_page_id = i.second;
            const TableSchema &table_schema = database_schema_.table_schema_map[table_name];
            for (auto &&iter : BPlusTreeSelect(id_page_id, nullptr, nullptr, false))
            {
                char *mem = BPlusTreeSearch(database_schema_.table_schema_map[table_name].root_page_id, iter, false);
                size_t id = -1;
                auto column_map = toTokenMap(mem, table_schema, kSizeOfSizeT, &id);
                for (auto &&pair : table_schema.column_schema_map)
                {
                    const ColumnSchema &column_schema = pair.second;
                    if (!column_schema.be_reference_set.empty())
                    {
                        size_t size = column_schema.data_type ? column_schema.data_type : kSizeOfSizeT;
                        char *key = new char[size + kSizeOfBool + kSizeOfSizeT];
                        for (auto &&i : column_schema.be_reference_set)
                        {
                            std::string reference_table_name = i.first;
                            std::string reference_column_name = i.second;
                            serilization({column_map[pair.first]}, {size - kSizeOfBool}, key);
                            const IndexSchema &index_schema = database_schema_.table_schema_map[reference_table_name].column_schema_map[reference_column_name].index_schema;
                            if (BPlusTreeSearch(index_schema.root_page_id, key, true))
                            {
                                delete[] key;
                                for (auto &&i : table_id_page_id_map)
                                {
                                    BPlusTreeRemove(i.second);
                                }
                                throw Error(kForeignkeyConstraintError, "");
                            }
                        }
                        delete[] key;
                    }
                }
            }
        }
        for (auto &&i : table_id_page_id_map)
        {
            std::string table_name = i.first;
            size_t id_page_id = i.second;
            const TableSchema &table_schema = database_schema_.table_schema_map[table_name];
            for (auto &&iter : BPlusTreeSelect(id_page_id, nullptr, nullptr, false))
            {
                char *mem = BPlusTreeSearch(database_schema_.table_schema_map[table_name].root_page_id, iter, false);
                size_t id = -1;
                auto column_map = toTokenMap(mem, table_schema, kSizeOfSizeT, &id);
                for (auto &&pair : table_schema.column_schema_map)
                {
                    const ColumnSchema &column_schema = pair.second;
                    size_t size = column_schema.data_type ? column_schema.data_type : kSizeOfSizeT;
                    char *key = new char[size + kSizeOfBool + kSizeOfSizeT];
                    serilization({column_map[pair.first]}, {size - kSizeOfBool}, key);
                    size_t val = column_map[pair.first].num;
                    std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, key + size + kSizeOfBool);
                    if (column_schema.index_schema.root_page_id != -1)
                    {
                        size_t root_page_id = column_schema.index_schema.root_page_id;
                        BPlusTreeDelete(root_page_id, key, &root_page_id);
                        database_schema_.table_schema_map[table_name].column_schema_map[pair.first].index_schema.root_page_id = root_page_id;
                    }
                    if (database_schema_.table_schema_map[table_name].index_schema_map.find({pair.first}) != database_schema_.table_schema_map[table_name].index_schema_map.end())
                    {
                        for (auto &&index_schema_pair : database_schema_.table_schema_map[table_name].index_schema_map[{pair.first}])
                        {
                            auto &index_schema = index_schema_pair.second;
                            size_t root_page_id = index_schema.root_page_id;
                            BPlusTreeDelete(root_page_id, key, &root_page_id);
                            database_schema_.table_schema_map[table_name].index_schema_map[{pair.first}][index_schema_pair.first].root_page_id = root_page_id;
                        }
                    }
                    delete[] key;
                }
                size_t page_id = database_schema_.table_schema_map[table_name].root_page_id;
                BPlusTreeDelete(page_id, iter, &page_id);
                database_schema_.table_schema_map[table_name].root_page_id = page_id;
            }
        }
        for (auto &&i : table_id_page_id_map)
        {
            BPlusTreeRemove(i.second);
        }
        updateDatabaseSchema();
        result_.type = kDeleteResult;
    }
}

void GDBE::deleteRecursive(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition_map, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, const std::unordered_set<std::string> &select_table_name_set, const std::unordered_set<std::string> &delete_table_name_set, std::unordered_map<std::string, size_t> &table_id_page_id_map)
{
    deleteRecursiveAux(table_index_condition_map, table_condition_map, select_table_name_set, {}, {}, delete_table_name_set, table_id_page_id_map, {});
}

void GDBE::deleteRecursiveAux(const std::unordered_map<std::string, std::pair<IndexSchema, std::pair<Token, Token>>> &table_index_condition_map, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, std::unordered_set<std::string> remain_table_name_set, std::unordered_set<std::string> already_table_name_set, std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column_map, const std::unordered_set<std::string> &delete_table_name_set, std::unordered_map<std::string, size_t> &table_id_page_id_map, std::unordered_map<std::string, size_t> table_id_map)
{
    if (!remain_table_name_set.empty())
    {
        std::string table_name = *remain_table_name_set.begin();
        remain_table_name_set.erase(remain_table_name_set.begin());
        already_table_name_set.insert(table_name);
        const TableSchema &table_schema = database_schema_.table_schema_map[table_name];
        const auto &index_condition_map_iter = table_index_condition_map.find(table_name);
        if (index_condition_map_iter != table_index_condition_map.end() && index_condition_map_iter->second.first.root_page_id != -1)
        {
            std::pair<IndexSchema, std::pair<Token, Token>> index_condition = index_condition_map_iter->second;
            std::pair<Token, Token> condition = index_condition.second;
            const IndexSchema &index_schema = index_condition.first;
            std::string column_name = index_schema.column_name;
            const ColumnSchema &column_schema = database_schema_.table_schema_map[table_name].column_schema_map[column_name];
            int data_type = column_schema.data_type;
            size_t size = data_type == 0 ? kSizeOfLong : data_type;
            Token token;
            if (data_type == 0)
            {
                convertInt(condition.first);
                convertInt(condition.second);
            }
            else
            {
                convertString(condition.first);
                convertString(condition.second);
            }
            char *begin_key = nullptr, *end_key = nullptr;
            if (condition.first)
            {
                begin_key = new char[size + kSizeOfBool];
                serilization({condition.first}, {size}, begin_key);
            }
            if (condition.second)
            {
                end_key = new char[size + kSizeOfBool];
                serilization({condition.second}, {size}, end_key);
            }
            size_t index_page_id = index_schema.root_page_id;
            PageSchema temp_page_schema(true, 0, -1, -1, kSizeOfSizeT + kSizeOfBool, kSizeOfSizeT + kSizeOfBool, 0, false);
            size_t temp_page_id = createNewPage(temp_page_schema);
            char *temp_key = new char[kSizeOfSizeT + kSizeOfBool];
            bool null = false;
            for (auto &&i : BPlusTreeSelect(index_page_id, begin_key, end_key, true))
            {
                size_t id = *reinterpret_cast<const size_t *>(i + size + kSizeOfBool + kSizeOfSizeT);
                std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, temp_key);
                std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, temp_key + kSizeOfBool);
                BPlusTreeInsert(temp_page_id, temp_key, nullptr, true, &temp_page_id);
            }
            delete[] temp_key;
            for (auto &&id_iter : BPlusTreeSelect(temp_page_id, nullptr, nullptr, false))
            {
                size_t id = *reinterpret_cast<const size_t *>(id_iter + kSizeOfBool);
                bool is_true = true;
                char *iter = BPlusTreeSearch(table_schema.root_page_id, id_iter, false);
                std::unordered_map<std::string, Token> column_token_map = toTokenMap(iter, table_schema, size, &id);
                table_id_map[table_name] = id;
                table_column_map[table_name] = column_token_map;
                for (const auto &i : table_condition_map)
                {
                    if (i.first.find(table_name) == i.first.end())
                        continue;
                    else
                    {
                        bool flag = true;
                        for (const auto &j : i.first)
                        {
                            if (already_table_name_set.find(j) == already_table_name_set.end())
                            {
                                flag = false;
                                break;
                            }
                        }
                        if (flag)
                        {
                            for (const auto &r : i.second)
                            {
                                Node result_node = eval(r, table_column_map, false);
                                if (result_node.token.token_type != kNum || !result_node.token.num)
                                {
                                    is_true = false;
                                    break;
                                }
                            }
                        }
                    }
                    if (!is_true)
                        break;
                }
                if (is_true)
                    deleteRecursiveAux(table_index_condition_map, table_condition_map, remain_table_name_set, already_table_name_set, table_column_map, delete_table_name_set, table_id_page_id_map, table_id_map);
            }
            BPlusTreeRemove(temp_page_id);
            if (begin_key)
                delete[] begin_key;
            if (end_key)
                delete[] end_key;
        }
        else
        {
            for (auto &&iter : BPlusTreeSelect(table_schema.root_page_id, nullptr, nullptr, false))
            {
                bool is_true = true;
                size_t id = -1;
                std::unordered_map<std::string, Token> column_token_map = toTokenMap(iter, table_schema, kSizeOfSizeT, &id);
                table_id_map[table_name] = id;
                table_column_map[table_name] = column_token_map;

                for (const auto &i : table_condition_map)
                {
                    if (i.first.find(table_name) == i.first.end())
                        continue;
                    else
                    {
                        bool flag = true;
                        for (const auto &j : i.first)
                        {
                            if (already_table_name_set.find(j) == already_table_name_set.end())
                            {
                                flag = false;
                                break;
                            }
                        }
                        if (flag)
                        {
                            for (const auto &r : i.second)
                            {
                                Node result_node = eval(r, table_column_map, false);
                                if (result_node.token.token_type != kNum || !result_node.token.num)
                                {
                                    is_true = false;
                                    break;
                                }
                            }
                        }
                    }
                    if (!is_true)
                        break;
                }
                if (is_true)
                    deleteRecursiveAux(table_index_condition_map, table_condition_map, remain_table_name_set, already_table_name_set, table_column_map, delete_table_name_set, table_id_page_id_map, table_id_map);
            }
        }
    }
    else
    {
        if (table_id_page_id_map.begin()->second == -1)
        {
            PageSchema id_page_schema(true, 0, -1, -1, kSizeOfSizeT + kSizeOfBool, kSizeOfSizeT + kSizeOfBool, 0, true);
            for (auto &&i : table_id_page_id_map)
            {
                i.second = createNewPage(id_page_schema);
            }
        }
        char *id_ptr = new char[kSizeOfSizeT + kSizeOfBool];
        bool null = false;
        for (auto &&i : table_id_page_id_map)
        {
            size_t id = table_id_map[i.first];
            std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, id_ptr);
            std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, id_ptr + kSizeOfBool);
            size_t root_page_id = table_id_page_id_map[i.first];
            BPlusTreeInsert(root_page_id, id_ptr, nullptr, true, &root_page_id);
            table_id_page_id_map[i.first] = root_page_id;
        }
        delete[] id_ptr;
    }
}
void GDBE::execCreateIndex(const Node &index_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    std::string index_name = index_node.children.front().token.str;

    std::string table_name = getTableName(index_node.children[1], database_name_);
    if (database_schema_.table_schema_map.find(table_name) == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);

    if (index_node.children[2].children.size() != 1)
        throw Error(kSyntaxTreeError, index_node.children[2].token.str);
    std::string column_name = getColumnName(index_node.children.back().children.front(), database_name_, table_name);
    if (database_schema_.table_schema_map[table_name].column_schema_map.find(column_name) == database_schema_.table_schema_map[table_name].column_schema_map.end())
        throw Error(kColumnNotExistError, column_name);
    if (database_schema_.table_schema_map[table_name].index_column_map.find(index_name) != database_schema_.table_schema_map[table_name].index_column_map.end())
        throw Error(kDuplicateIndexError, index_name);
    TableSchema &table_schema = database_schema_.table_schema_map[table_name];
    ColumnSchema &column_schema = table_schema.column_schema_map[column_name];
    int data_type = column_schema.data_type;
    size_t index_size = data_type == 0 ? kSizeOfLong + kSizeOfBool : data_type + kSizeOfBool;
    size_t key_size = index_size + kSizeOfSizeT;
    PageSchema index_page_schema(true, 0, -1, -1, key_size, index_size, kSizeOfSizeT, !data_type);
    IndexSchema index_schema;
    index_schema.column_name = column_name;
    index_schema.root_page_id = createNewPage(index_page_schema);

    auto &&iterator = BPlusTreeSelect(table_schema.root_page_id, nullptr, nullptr, false);
    auto &&begin_iter = iterator.begin();
    auto &&end_iter = iterator.end();
    size_t root_page_id = -1;
    for (auto &&iter = begin_iter; iter != end_iter; ++iter)
    {
        size_t id = -1;
        std::unordered_map<std::string, Token> column_token_map = toTokenMap(*iter, table_schema, kSizeOfSizeT, &id);
        Token index_token = column_token_map[column_name];
        char *key_ptr = new char[key_size];
        char *values_ptr = new char[kSizeOfSizeT];
        serilization({index_token}, {index_size - kSizeOfBool}, key_ptr);
        std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, key_ptr + index_size);
        std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, values_ptr);
        BPlusTreeInsert(index_schema.root_page_id, key_ptr, values_ptr, false, &root_page_id);
        if (root_page_id != -1)
        {
            index_schema.root_page_id = root_page_id;
            root_page_id = -1;
        }
        delete[] key_ptr;
        delete[] values_ptr;
    }
    table_schema.index_schema_map[{column_name}][index_name] = index_schema;
    table_schema.index_column_map[index_name] = {column_name};
    updateDatabaseSchema();
    result_.type = kCreateIndexResult;
}

void GDBE::execShowIndex(const Node &index_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    std::vector<std::string> string_vector;
    std::string table_name = getTableName(index_node.children.front(), database_name_);
    if (database_schema_.table_schema_map.find(table_name) == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    for (const auto &i : database_schema_.table_schema_map[table_name].index_column_map)
    {
        string_vector.push_back(i.first);
    }
    result_.type = kShowIndexResult;
    result_.string_vector_vector.push_back(std::move(string_vector));
}

void GDBE::execDropIndex(const Node &index_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    std::string index_name = index_node.children.front().token.str;
    const Node &name_node = index_node.children.back();
    std::string table_name = getTableName(name_node, database_name_);
    auto table_iter = database_schema_.table_schema_map.find(table_name);
    if (table_iter == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    if (table_iter->second.index_column_map.find(index_name) == table_iter->second.index_column_map.end())
        throw Error(kIndexNotExistError, index_name);
    auto column_set = table_iter->second.index_column_map[index_name];
    auto index_iter = table_iter->second.index_schema_map.find(column_set);
    if (index_iter == table_iter->second.index_schema_map.end())
        throw Error(kIndexNotExistError, index_name);
    size_t page_id = index_iter->second[index_name].root_page_id;
    BPlusTreeRemove(page_id);
    database_schema_.table_schema_map[table_name].index_column_map.erase(index_name);
    database_schema_.table_schema_map[table_name].index_schema_map[column_set].erase(index_name);
    if (database_schema_.table_schema_map[table_name].index_schema_map[column_set].empty())
        database_schema_.table_schema_map[table_name].index_schema_map.erase(column_set);
    updateDatabaseSchema();
    result_.type = kDropIndexResult;
}

void GDBE::execAlter(const Node &alter_node)
{
    throw Error(kSyntaxTreeError, alter_node.token.str);
}

void GDBE::execUpdate(const Node &update_node)
{
    throw Error(kSyntaxTreeError, update_node.token.str);
}

size_t GDBE::getExprDataType(const Node &node)
{
    if (node.token.token_type == kName)
    {
        int data_type = database_schema_.table_schema_map[node.children.front().token.str].column_schema_map[node.children.back().token.str].data_type;
        return data_type;
    }
    else if (node.token.token_type == kString)
    {
        return node.token.str.size() + 1;
    }
    else
    {
        return 0;
    }
}

size_t GDBE::getValueSize(const std::unordered_map<std::string, ColumnSchema> &column_schema_map)
{
    size_t size = 0;
    size += (column_schema_map.size()) * kSizeOfBool;
    for (auto &&i : column_schema_map)
    {
        if (i.second.data_type == 0)
            size += kSizeOfLong;
        else
            size += i.second.data_type;
    }
    return size;
}

void GDBE::updateDatabaseSchema()
{
    size_t page_num = (getSize(database_schema_) - 1) / kPageSize + 1;
    while (page_num > database_schema_.page_vector.size())
    {
        database_schema_.page_vector.push_back(getFreePage());
        page_num = (getSize(database_schema_) - 1) / kPageSize + 1;
    }
    std::vector<PagePtr> page_ptr_vector;
    for (auto &&i : database_schema_.page_vector)
    {
        page_ptr_vector.push_back(buffer_pool_.getPage(i));
    }
    Stream stream(page_ptr_vector);
    stream << database_schema_;
    for (size_t i = 0; i < page_ptr_vector.size(); ++i)
        file_system_.write(database_schema_.page_vector[i], page_ptr_vector[i]);
}

std::pair<IndexSchema, std::pair<Token, Token>> GDBE::getCondition(std::vector<Node> &expr_vector, bool *rc)
{
    std::unordered_map<IndexSchema, std::pair<Token, Token>, MyIndexSchemaHashFunction, MyIndexSchemaEqualFunction> index_condition_map;
    for (auto &i : expr_vector)
    {
        if (isIndexCondition(i))
        {
            const Node &name_node = i.children.front();
            IndexSchema index_schema;
            if (database_schema_.table_schema_map[name_node.children.front().token.str].column_schema_map[name_node.children.back().token.str].index_schema.root_page_id != -1)
                index_schema = database_schema_.table_schema_map[name_node.children.front().token.str].column_schema_map[name_node.children.back().token.str].index_schema;
            else
                index_schema = database_schema_.table_schema_map[name_node.children.front().token.str].index_schema_map[{name_node.children.back().token.str}].begin()->second;
            int data_type = database_schema_.table_schema_map[name_node.children.front().token.str].column_schema_map[name_node.children.back().token.str].data_type;
            if (index_schema.root_page_id == -1)
            {
                const auto &iter = database_schema_.table_schema_map[name_node.children.front().token.str].index_schema_map.find({name_node.children.back().token.str});
                if (iter == database_schema_.table_schema_map[name_node.children.front().token.str].index_schema_map.end())
                    continue;
                index_schema = (*iter->second.begin()).second;
            }
            std::pair<Token, Token> condition;
            if (index_condition_map.find(index_schema) != index_condition_map.end())
                condition = index_condition_map[index_schema];
            if (data_type == 0)
                convertInt(i.children.back().token);
            else if (data_type > 0)
                convertString(i.children.back().token);
            if ((i.token.token_type == kGreater || i.token.token_type == kGreaterEqual))
            {
                if (condition.first.token_type == kNone)
                    condition.first = i.children.back().token;
                else if (data_type == 0 && i.children.back().token.num > condition.first.num)
                    condition.first = i.children.back().token;
                else if (data_type > 0 && i.children.back().token.str > condition.first.str)
                    condition.first = i.children.back().token;
            }
            else if ((i.token.token_type == kLess || i.token.token_type == kLessEqual))
            {
                if (condition.second.token_type == kNone)
                    condition.second = i.children.back().token;
                else if (data_type == 0 && i.children.back().token.num < condition.first.num)
                    condition.second = i.children.back().token;
                else if (data_type > 0 && i.children.back().token.str < condition.first.str)
                    condition.second = i.children.back().token;
            }
            else if (i.token.token_type == kEqual)
            {
                condition.first = condition.second = i.children.back().token;
            }
            if (condition.first.token_type == kNum && condition.second.token_type == kNum && condition.first.num > condition.second.num)
                *rc = false;
            if (condition.first.token_type == kString && condition.second.token_type == kString && condition.first.str > condition.second.str)
                *rc = false;
            index_condition_map[index_schema] = condition;
        }
    }
    if (index_condition_map.empty())
        return {IndexSchema(), {Token(kNull), Token(kNull)}};
    else
        return {index_condition_map.begin()->first, index_condition_map.begin()->second};
}

bool GDBE::isIndexCondition(Node &node)
{
    if (node.token.token_type != kGreater && node.token.token_type != kGreaterEqual && node.token.token_type != kLess && node.token.token_type != kLessEqual && node.token.token_type != kEqual)
        return false;
    Node &left_node = node.children.front();
    Node &right_node = node.children.back();
    if (left_node.token.token_type == kName && (right_node.token.token_type == kNull || right_node.token.token_type == kNum || right_node.token.token_type == kString))
    {
        if (database_schema_.table_schema_map[left_node.children.front().token.str].column_schema_map[left_node.children.back().token.str].index_schema.root_page_id != -1 || database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.find({left_node.children.back().token.str}) != database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.end())
            return true;
        else
            return false;
    }
    else if (right_node.token.token_type == kName && (left_node.token.token_type == kNull || left_node.token.token_type == kNum || left_node.token.token_type == kString))
    {
        std::swap(right_node, left_node);
        if (database_schema_.table_schema_map[left_node.children.front().token.str].column_schema_map[left_node.children.back().token.str].index_schema.root_page_id != -1 || database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.find({left_node.children.back().token.str}) != database_schema_.table_schema_map[left_node.children.front().token.str].index_schema_map.end())
            return true;
        else
            return false;
    }
    else
        return false;
}