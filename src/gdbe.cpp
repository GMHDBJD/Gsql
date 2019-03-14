#include <fstream>
#include "gdbe.h"
#include "stream.h"

void GDBE::exec(SyntaxTree syntax_tree)
{
    syntax_tree_ = std::move(syntax_tree);
    query_optimizer_.optimizer(&syntax_tree);
    result_.string_vector_vector_.clear();
    execRoot(syntax_tree_.root_);
}

Result GDBE::getResult()
{
    if (result_.type_ != kSelectResult)
    {
        Result temp_result = std::move(result_);
        result_.type_ = kNoneResult;
        result_.string_vector_vector_.clear();
        return std::move(temp_result);
    }
    else
    {
        return result_;
    }
}

void GDBE::execRoot(const Node &node)
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
        result_.type_ = kExitResult;
        break;
    default:
        break;
    }
}

void GDBE::execCreate(const Node &create_node)
{
    const Node &next_node = create_node.childern.front();
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
    const Node &string_node = database_node.childern.front();
    if (!file_system_.exists(kDatabaseDir))
        file_system_.create_directory(kDatabaseDir);
    if (file_system_.exists(kDatabaseDir + string_node.token.str))
        throw Error(kDatabaseExistError, string_node.token.str);
    DatabaseSchema new_database_schema;
    new_database_schema.page_vector.push_back(0);
    std::vector<PagePtr> page_ptr_vector;
    page_ptr_vector.push_back(buffer_pool_.getPage(0, true));
    Stream stream(page_ptr_vector);
    stream << new_database_schema;
    std::fstream file(string_node.token.str, std::fstream::out);
    file_system_.swap(file);
    file_system_.write(0, page_ptr_vector[0]);
    file_system_.swap(file);
    file.close();
    file_system_.rename(string_node.token.str, kDatabaseDir + string_node.token.str);
    result_.type_ = kCreateDatabaseResult;
}

void GDBE::execShow(const Node &show_node)
{
    const Node &next_node = show_node.childern.front();
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
        result_.type_ = kShowDatabasesResult;
        return;
    }
    std::vector<std::string> string_vector;
    for (auto &entry : fs::directory_iterator(kDatabaseDir))
    {
        string_vector.push_back(entry.path().filename().string());
    }
    result_.type_ = kShowDatabasesResult;
    result_.string_vector_vector_.push_back(std::move(string_vector));
}

void GDBE::execUse(const Node &use_node)
{
    const Node &string_node = use_node.childern.front().childern.front();
    if (string_node.token.str != database_name_)
    {
        if (!file_system_.exists(kDatabaseDir + string_node.token.str))
            throw Error(kDatabaseNotExistError, string_node.token.str);
        file_system_.setFile(kDatabaseDir + string_node.token.str);
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
    result_.type_ = kUseResult;
}

void GDBE::execDrop(const Node &drop_node)
{
    const Node &next_node = drop_node.childern.front();
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
    const Node &string_node = database_node.childern.front().childern.front();
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
    result_.type_ = kDropDatabaseResult;
}

void GDBE::execCreateTable(const Node &table_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    std::string table_name = getTableName(table_node.childern.front(), database_name_);
    if (database_schema_.table_schema_map.find(table_name) != database_schema_.table_schema_map.end())
        throw Error(kTableExistError, table_name);
    TableSchema new_table_schema;
    const Node &columns_node = table_node.childern.back();
    for (const auto &node : columns_node.childern)
    {
        if (node.token.token_type == kColumn)
        {
            ColumnSchema new_column_schema;
            std::string column_name = getColumnName(node.childern.front(), database_name_, table_name);
            switch (node.childern[1].token.token_type)
            {
            case kInt:
                new_column_schema.data_type = 0;
                break;
            case kChar:
            {
                if (node.childern[1].childern.empty())
                    new_column_schema.data_type = 1;
                else
                    new_column_schema.data_type = node.childern[1].childern.front().token.num;
                break;
            }
            default:
                throw Error(kSyntaxTreeError, node.childern[1].token.str);
            }
            for (size_t i = 2; i < node.childern.size(); i++)
            {
                switch (node.childern[i].token.token_type)
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
                    Node value_node = node.childern.back().childern.front();
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
                    break;
                }
                default:
                    throw Error(kSyntaxTreeError, node.childern.back().token.str);
                }
            }
            auto rc = new_table_schema.column_schema_map.insert({column_name, new_column_schema});
            if (!rc.second)
                throw Error(kDuplicateColumnError, column_name);
            if (node.childern.size() == 3)
                new_column_schema.not_null = true;
            new_table_schema.column_order_vector.push_back(column_name);
        }
    }
    for (const auto &node : columns_node.childern)
    {
        if (node.token.token_type == kForeign)
        {
            const Node &names_node = node.childern.front();
            std::string column_name = getColumnName(node.childern[0], database_name_, table_name);
            std::string reference_table_name = getTableName(node.childern[1], database_name_);
            std::string reference_column_name = getColumnName(node.childern[2], database_name_, reference_table_name);
            auto column_iter = new_table_schema.column_schema_map.find(column_name);
            if (column_iter == new_table_schema.column_schema_map.end())
                throw Error(kAddForeiginError, "");
            auto reference_table_iter = database_schema_.table_schema_map.find(reference_table_name);
            if (reference_table_iter == database_schema_.table_schema_map.end())
                throw Error(kAddForeiginError, "");
            auto reference_column_iter = reference_table_iter->second.column_schema_map.find(reference_column_name);
            if (reference_column_iter == reference_table_iter->second.column_schema_map.end())
                throw Error(kAddForeiginError, "");
            if (reference_column_iter->second.data_type != column_iter->second.data_type || !reference_column_iter->second.not_null)
                throw Error(kAddForeiginError, "");
            if (!column_iter->second.reference_table_name.empty() && column_iter->second.reference_table_name != reference_table_name)
                throw Error(kAddForeiginError, "");
            if (!column_iter->second.reference_column_name.empty() && column_iter->second.reference_column_name != reference_column_name)
                throw Error(kAddForeiginError, "");
            column_iter->second.reference_column_name = reference_table_name;
            column_iter->second.reference_table_name = reference_column_name;
        }
        else if (node.token.token_type == kPrimary)
        {
            if (!new_table_schema.primary_set.empty())
                throw Error(kMultiplePrimaryKeyError, "");
            const Node &names_node = node.childern.front();
            for (const auto &name_node : names_node.childern)
            {
                std::string column_name = getColumnName(name_node, database_name_, table_name);
                if (new_table_schema.column_schema_map.find(column_name) == new_table_schema.column_schema_map.end())
                    throw Error(kColumnNotExistError, column_name);
                auto rc = new_table_schema.primary_set.insert(column_name);
                if (!rc.second)
                    throw Error(kDuplicateColumnError, column_name);
            }
        }
    }
    new_table_schema.max_id = 0;
    new_table_schema.root_page_num = createNewPage();
    database_schema_.table_schema_map[table_name] = new_table_schema;
    updateDatabaseSchema();
    result_.type_ = kCreateTableResult;
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
    result_.type_ = kShowTablesResult;
    result_.string_vector_vector_.push_back(std::move(string_vector));
}

void GDBE::execDropTable(const Node &table_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    const Node &name_node = table_node.childern.front();
    std::string table_name = getTableName(name_node, database_name_);
    auto table_iter = database_schema_.table_schema_map.find(table_name);
    if (table_iter == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    database_schema_.table_schema_map.erase(table_iter->first);
    try
    {
        updateDatabaseSchema();
    }
    catch (const Error &error)
    {
        database_schema_.table_schema_map[table_iter->first] = table_iter->second;
    }
    result_.type_ = kDropTableResult;
}

void GDBE::execInsert(const Node &insert_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    const Node &name_node = insert_node.childern.front();
    std::string table_name = getTableName(name_node, database_name_);
    auto table_scheam_iter = database_schema_.table_schema_map.find(table_name);
    if (table_scheam_iter == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    std::unordered_map<std::string, size_t> name_pos_map;
    if (insert_node.childern.size() == 3)
    {
        int pos = 0;
        for (auto &&name_node : insert_node.childern[1].childern)
        {
            std::string column_name = getColumnName(name_node, database_name_, table_name);
            if (database_schema_.table_schema_map[table_name].column_schema_map.find(column_name) == database_schema_.table_schema_map[table_name].column_schema_map.end())
                throw Error(kColumnNotExistError, column_name);
            if (name_pos_map.find(column_name) == name_pos_map.end())
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
    for (const auto &exprs_node : insert_node.childern.back().childern)
    {
        ++row;
        if (exprs_node.childern.size() != name_pos_map.size())
            throw Error(kColumnCountNotMatch, std::to_string(row));
        std::vector<Token> values;
        std::unordered_map<std::string, Token> name_value_map;
        std::vector<size_t> values_size;
        values_size.push_back(0);
        size_t id = table_scheam_iter->second.max_id++;
        values.push_back(Token(kNum, std::to_string(id), id));
        for (auto &&column_name : database_schema_.table_schema_map[table_name].column_order_vector)
        {
            const ColumnSchema &column_schema = database_schema_.table_schema_map[table_name].column_schema_map[column_name];
            size_t pos = name_pos_map[column_name];
            Token value_token = eval(exprs_node.childern[pos].childern.front(), table_name, name_value_map);
            if (column_schema.data_type == 0 && value_token.token_type == kString)
            {
                try
                {
                    convertInt(value_token);
                }
                catch (const Error &e)
                {
                    throw Error(kIncorrectIntegerValue, "\"" + value_token.str + "\"" + " for column " + column_name + " at row " + std::to_string(row));
                }
            }
            else if (column_schema.data_type > 0 && value_token.token_type == kNum)
            {
                convertString(value_token);
            }
            values.push_back(value_token);
            values_size.push_back(column_schema.data_type);
        }
        char *key_ptr = new char[kSizeOfSizeT];
        std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, key_ptr);
        size_t size = getValueSize(values_size);
        char *values_ptr = new char[size];
        serilization(values, values_size, values_ptr);
    }
}

Token GDBE::eval(const Node &node, const std::string &table_name, const std::unordered_map<std::string, Token> &name_value_map)
{
    switch (node.token.token_type)
    {
    case kAnd:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num && right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kNot:
    {
        Token next_token = eval(node.childern.front(), table_name, name_value_map);
        convertInt(next_token);
        if (next_token.token_type == kNum)
        {
            int result = !next_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (next_token.token_type == kNull)
            return next_token;
        else
            throw Error(kOperationError, "");
    }
    case kOr:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num || right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else if (left_token.token_type == kNum && right_token.token_type == kNull)
            return Token(kNum, std::to_string(left_token.num != 0), left_token.num != 0);
        else if (left_token.token_type == kNull && right_token.token_type == kNum)
            return Token(kNum, std::to_string(right_token.num != 0), right_token.num != 0);
        else
            throw Error(kOperationError, "");
    }
    case kLess:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num < right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kGreater:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num > right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kLessEqual:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num <= right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kGreaterEqual:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num >= right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kEqual:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num == right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kNotEqual:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num != right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kPlus:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num + right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kMinus:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num - right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kMultiply:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num * right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kDivide:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num / right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kMod:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num % right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kShiftLeft:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num << right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kShiftRight:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num >> right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kBitsExclusiveOr:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num ^ right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kBitsAnd:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num & right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kBitsOr:
    {
        Token left_token = eval(node.childern.front(), table_name, name_value_map);
        Token right_token = eval(node.childern.back(), table_name, name_value_map);
        convertInt(left_token);
        convertInt(right_token);
        if (left_token.token_type == kNum && right_token.token_type == kNum)
        {
            int result = left_token.num | right_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (left_token.token_type == kNull || right_token.token_type == kNull)
            return Token(kNull, "NULL");
        else
            throw Error(kOperationError, "");
    }
    case kBitsNot:
    {
        Token next_token = eval(node.childern.front(), table_name, name_value_map);
        convertInt(next_token);
        if (next_token.token_type == kNum)
        {
            int result = ~next_token.num;
            return Token(kNum, std::to_string(result), result);
        }
        else if (next_token.token_type == kNull)
            return next_token;
        else
            throw Error(kOperationError, "");
    }
    case kNum:
    case kNull:
    case kString:
        return node.token;
    case kName:
    {
        std::string column_name = getColumnName(node, database_name_, table_name);
        const auto &iter = database_schema_.table_schema_map[table_name].column_schema_map.find(column_name);
        if (iter == database_schema_.table_schema_map[column_name].column_schema_map.end())
            throw Error(kColumnNotExistError, column_name);
        const auto &name_value_map_iter = name_value_map.find(column_name);
        if (name_value_map_iter != name_value_map.end())
            return name_value_map_iter->second;
        else
        {
            if (iter->second.data_type == 0)
                return Token(kNum, iter->second.default_value, std::stoi(iter->second.default_value));
            else if (iter->second.data_type > 0)
                return Token(kString, iter->second.default_value);
        }
    }
    default:
        throw Error(kSyntaxTreeError, node.token.str);
    }
}

void GDBE::execExplain(const Node &explain_node)
{
    if (database_name_.empty())
        throw Error(kNoDatabaseSelectError, "");
    const Node &name_node = explain_node.childern.front();
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
        result_.string_vector_vector_.push_back(std::move(string_vector));
    }
    result_.type_ = kExplainResult;
}