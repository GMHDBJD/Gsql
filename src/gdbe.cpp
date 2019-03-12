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
    std::vector<PagePtr> page_ptr_vector = new_database_schema.toPage();
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
        PagePtr root_page = buffer_pool_.getPage(0);
        Stream stream(root_page.get(), kPageSize);
        DatabaseSchema new_database_schema;
        stream >> new_database_schema.page_vector;
        std::vector<PagePtr> page_ptr_vector;
        for (auto &&i : new_database_schema.page_vector)
        {
            page_ptr_vector.push_back(buffer_pool_.getPage(i));
        }
        new_database_schema.pageTo(page_ptr_vector);
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
    std::string table_name = getTableName(table_node.childern.front());
    if (database_schema_.table_schema_map.find(table_name) != database_schema_.table_schema_map.end())
        throw Error(kTableExistError, table_name);
    TableSchema new_table_schema;
    const Node &columns_node = table_node.childern.back();
    for (const auto &node : columns_node.childern)
    {
        if (node.token.token_type == kColumn)
        {
            ColumnSchema new_column_schema;
            std::string column_name = getColumnName(node.childern.front(), table_name);
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
                    new_column_schema.data_type = node.childern[1].childern.size();
                break;
            }
            default:
                throw Error(kSyntaxTreeError, node.childern[1].token.str);
            }
            switch (node.childern.size())
            {
            case 2:
                break;
            case 3:
            {
                switch (node.childern.back().token.token_type)
                {
                case kNotNull:
                {
                    new_column_schema.not_null = true;
                    new_column_schema.null_default = false;
                    if (new_column_schema.data_type == 0)
                        new_column_schema.default_value = "0";
                    else
                        new_column_schema.default_value = "";
                    break;
                }
                case kDefault:
                {
                    new_column_schema.null_default = false;
                    Node value_node = node.childern.back().childern.front();
                    if (value_node.token.token_type == kInt && new_column_schema.data_type != 0 || value_node.token.token_type == kChar && new_column_schema.data_type <= 0)
                    {
                        throw Error(kInvalidValueError, column_name);
                    }
                    else
                    {
                        new_column_schema.default_value = value_node.token.str;
                    }
                    break;
                }
                default:
                    throw Error(kSyntaxTreeError, node.childern.back().token.str);
                }
                break;
            }
            case 4:
            {
                new_column_schema.not_null = true;
                new_column_schema.null_default = false;
                Node value_node = node.childern.back().childern.front();
                if (value_node.token.token_type == kInt && new_column_schema.data_type != 0 || value_node.token.token_type == kChar && new_column_schema.data_type <= 0)
                {
                    throw Error(kInvalidValueError, column_name);
                }
                else
                {
                    new_column_schema.default_value = value_node.token.str;
                }
                break;
            }
            default:
                throw Error(kSyntaxTreeError, node.childern.back().token.str);
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
            std::string column_name = getColumnName(node.childern[0], table_name);
            std::string reference_table_name = getTableName(node.childern[1]);
            std::string reference_column_name = getColumnName(node.childern[2], reference_table_name);
            auto column_iter = new_table_schema.column_schema_map.find(column_name);
            if (column_iter == new_table_schema.column_schema_map.end())
                throw Error(kAddForeiginError, "");
            auto reference_table_iter = database_schema_.table_schema_map.find(reference_table_name);
            if (reference_table_iter == database_schema_.table_schema_map.end())
                throw Error(kAddForeiginError, "");
            auto reference_column_iter = reference_table_iter->second.column_schema_map.find(column_name);
            if (reference_column_iter == reference_table_iter->second.column_schema_map.end())
                throw Error(kAddForeiginError, "");
            if (reference_column_iter->second.data_type != column_iter->second.data_type || reference_column_iter->second.not_null)
                throw Error(kAddForeiginError, "");
            if (!column_iter->second.reference_table_name.empty() && column_iter->second.reference_table_name != reference_table_name)
                throw Error(kAddForeiginError, "");
            if (!column_iter->second.reference_column_name.empty() && column_iter->second.reference_column_name != reference_column_name)
                throw Error(kAddForeiginError, "");
            column_iter->second.reference_column_name = reference_table_name;
            column_iter->second.reference_table_name = reference_table_name;
        }
        else if (node.token.token_type == kPrimary)
        {
            if (!new_table_schema.primary_set.empty())
                throw Error(kMultiplePrimaryKeyError, "");
            const Node &names_node = node.childern.front();
            for (const auto &name_node : names_node.childern)
            {
                std::string column_name = getColumnName(name_node, table_name);
                if (new_table_schema.column_schema_map.find(column_name) == new_table_schema.column_schema_map.end())
                    throw Error(kColumnNotExistError, column_name);
                auto rc = new_table_schema.primary_set.insert(column_name);
                if (!rc.second)
                    throw Error(kDuplicateColumnError, column_name);
            }
        }
    }
    size_t root_page_num = createNewPage();
    new_table_schema.root_page_num = 2;
    database_schema_.table_schema_map[table_name] = new_table_schema;
    try
    {
        updateDatabaseSchema();
    }
    catch (const Error &error)
    {
        database_schema_.table_schema_map.erase(table_name);
        throw error;
    }
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
    std::string table_name = getTableName(name_node);
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
    std::string table_name = getTableName(name_node);
    if (database_schema_.table_schema_map.find(table_name) == database_schema_.table_schema_map.end())
        throw Error(kTableNotExistError, table_name);
    switch (insert_node.childern.size())
    {
    case 2:
    {
        std::unordered_map<std::string, Node> name_value_map;
        const Node &values_node = insert_node.childern.back();
        for (const auto &exprs_node : values_node.childern)
        {
            for (size_t i = 0; i < exprs_node.childern.size(); i++)
            {
                const auto &expr_node = exprs_node.childern[i];
                Node node = eval(expr_node.childern.front(), table_name, name_value_map);
            }
        }

        break;
    }
    default:
        break;
    }
}

Node GDBE::eval(const Node &node, const std::string &table_name, const std::unordered_map<std::string, Node> &name_value_map)
{
    switch (node.token.token_type)
    {
    case kAnd:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num && right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kNot:
    {
        Node next_node = eval(node.childern.front(), table_name, name_value_map);
        if (next_node.token.token_type == kNum)
        {
            int result = !next_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (next_node.token.token_type == kNull)
            return next_node;
        else
            throw Error(kOperationError, "");
    }
    case kOr:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num || right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else if (left_node.token.token_type == kNum && right_node.token.token_type == kNull)
            return Node(Token(kNum, std::to_string(left_node.token.num != 0), left_node.token.num != 0));
        else if (left_node.token.token_type == kNull && right_node.token.token_type == kNum)
            return Node(Token(kNum, std::to_string(right_node.token.num != 0), right_node.token.num != 0));
        else
            throw Error(kOperationError, "");
    }
    case kLess:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num < right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kGreater:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num > right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kLessEqual:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num <= right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kGreaterEqual:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num >= right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kEqual:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num == right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kNotEqual:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num != right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kPlus:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num + right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kMinus:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num - right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kMultiply:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num * right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kDivide:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num / right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kMod:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num % right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kShiftLeft:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num << right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kShiftRight:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num >> right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kBitsExclusiveOr:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num ^ right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kBitsAnd:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num & right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kBitsOr:
    {
        Node left_node = eval(node.childern.front(), table_name, name_value_map);
        Node right_node = eval(node.childern.back(), table_name, name_value_map);
        if (left_node.token.token_type == kNum && right_node.token.token_type == kNum)
        {
            int result = left_node.token.num | right_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (left_node.token.token_type == kNull || right_node.token.token_type == kNull)
            return Node(Token(kNull, "NULL"));
        else
            throw Error(kOperationError, "");
    }
    case kBitsNot:
    {
        Node next_node = eval(node.childern.front(), table_name, name_value_map);
        if (next_node.token.token_type == kNum)
        {
            int result = ~next_node.token.num;
            return Node(Token(kNum, std::to_string(result), result));
        }
        else if (next_node.token.token_type == kNull)
            return next_node;
        else
            throw Error(kOperationError, "");
    }
    case kNum:
    case kNull:
    case kString:
    {
        return node;
    }
    case kName:
    {
        std::string column_name = getColumnName(node, table_name);
        const auto &iter = database_schema_.table_schema_map[table_name].column_schema_map.find(column_name);
        if (iter == database_schema_.table_schema_map[column_name].column_schema_map.end())
            throw Error(kColumnNotExistError, column_name);
        const auto &name_value_map_iter = name_value_map.find(column_name);
        if (name_value_map_iter != name_value_map.end())
            return Node(name_value_map_iter->second);
        else
        {
            if (iter->second.data_type == 0)
                return Node(Token(kInt, iter->second.default_value, std::stoi(iter->second.default_value)));
            else if (iter->second.data_type > 0)
                return Node(Token(kChar, iter->second.default_value));
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
    std::string table_name = getTableName(name_node);
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
        string_vector.push_back(column_schema.default_value);
        string_vector.push_back(column_schema.reference_table_name);
        string_vector.push_back(column_schema.reference_column_name);
        result_.string_vector_vector_.push_back(string_vector);
    }
    result_.type_ = kExplainResult;
}