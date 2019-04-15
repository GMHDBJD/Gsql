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
    result_.string_vector_vector.clear();
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
        if (result_.first)
        {
            Iterator iterator = BPlusTreeSelect(result_.page_id, nullptr, nullptr, false);
            result_.begin_iter = iterator.begin();
            result_.end_iter = iterator.end();
            result_.iter = result_.begin_iter;
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
        }
        if (result_.iter == result_.end_iter)
        {
            Result temp_result;
            removeBPlusTree(result_.page_id);
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
    page_ptr_vector.push_back(buffer_pool_.getPage(0, true));
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
            const Node &names_node = node.children.front();
            for (const auto &name_node : names_node.children)
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
    size_t value_size = getValueSize(new_table_schema.column_schema_map);
    PageSchema new_page_schema(true, 0, -1, -1, kSizeOfSizeT + kSizeOfBool, 0, value_size);
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
    size_t page_id = table_iter->second.root_page_id;
    removeBPlusTree(page_id);
    database_schema_.table_schema_map.erase(table_iter->first);
    updateDatabaseSchema();
    database_schema_.table_schema_map[table_iter->first] = table_iter->second;
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
        values_size.push_back(0);
        size_t id = table_schema_iter->second.max_id++;
        values.push_back(Token(kNum, std::to_string(id), id));
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
        char *key_ptr = new char[kSizeOfSizeT + kSizeOfBool];
        bool null = false;
        std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, key_ptr);
        std::copy(reinterpret_cast<const char *>(&id), reinterpret_cast<const char *>(&id) + kSizeOfSizeT, key_ptr + kSizeOfBool);
        size_t size = getValueSize(table_schema_iter->second.column_schema_map);
        char *values_ptr = new char[size];
        serilization(values, values_size, values_ptr);
        size_t root_page_id = -1;
        BPlusTreeInsert(table_schema_iter->second.root_page_id, key_ptr, values_ptr, false, &root_page_id);
        if (root_page_id != -1)
            table_schema_iter->second.root_page_id = root_page_id;
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
                select_expr_node_vector.push_back(name_node);
            }
        }
    }
    if (columns_node.children.back().token.token_type == kExprs)
    {
        for (auto &expr_node : columns_node.children.back().children)
        {
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
        eval(i, {}, true);
    }
    std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> table_condition_map;
    std::unordered_map<std::string, std::pair<std::string, std::pair<Token, Token>>> table_index_condition_map;
    bool rc = partitionConditionForTable(condition_vector, &table_condition_map);
    if (!rc)
        result_.type = kNoneResult;
    else
    {
        for (auto &i : table_condition_map)
            if (i.first.size() == 1)
                table_index_condition_map[*i.first.begin()] = getCondition(i.second);
        selectRecursive(table_index_condition_map, table_condition_map, select_table_name_set, select_expr_node_vector, limit);
        result_.type = kSelectResult;
    }
}

void GDBE::selectRecursive(const std::unordered_map<std::string, std::pair<std::string, std::pair<Token, Token>>> &table_index_condition_map, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, const std::unordered_set<std::string> &select_table_name_set, const std::vector<Node> &select_expr_vector, size_t limit)
{
    selectRecursiveAux(table_index_condition_map, table_condition_map, select_table_name_set, {}, {}, select_expr_vector, limit);
}

void GDBE::selectRecursiveAux(const std::unordered_map<std::string, std::pair<std::string, std::pair<Token, Token>>> &table_index_condition_map, const std::unordered_map<std::unordered_set<std::string>, std::vector<Node>, MySetHashFunction> &table_condition_map, std::unordered_set<std::string> remain_table_name_set, std::unordered_set<std::string> already_table_name_set, std::unordered_map<std::string, std::unordered_map<std::string, Token>> table_column_map, const std::vector<Node> &select_expr_vector, size_t limit)
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
        char *begin_key = nullptr, *end_key = nullptr;
        size_t page_id = table_schema.root_page_id;
        size_t size = kSizeOfSizeT;
        if (index_condition_map_iter != table_index_condition_map.end() && !index_condition_map_iter->second.first.empty())
        {
            std::pair<std::string, std::pair<Token, Token>> index_condition = index_condition_map_iter->second;
            std::string index_name = index_condition.first;
            std::pair<Token, Token> condition = index_condition.second;
            const IndexSchema &index_schema = database_schema_.table_schema_map[table_name].index_schema_map[index_name];
            std::string column_name = index_schema.column_name;
            const ColumnSchema &column_schema = database_schema_.table_schema_map[table_name].column_schema_map[column_name];
            int data_type = column_schema.data_type;
            size = data_type == 0 ? kSizeOfLong : data_type;
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
            begin_key = new char[size + kSizeOfBool];
            end_key = new char[size + kSizeOfBool];
            serilization({condition.first}, {size + kSizeOfBool}, begin_key);
            serilization({condition.second}, {size + kSizeOfBool}, end_key);
            page_id = index_schema.root_page_id;
        }
        for (auto &&i : BPlusTreeSelect(page_id, begin_key, end_key, false))
        {
            bool is_true = true;
            std::unordered_map<std::string, Token> column_token_map = toTokenMap(i, table_schema, size);
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
                if (begin_key != nullptr)
                    delete[] begin_key;
                if (end_key != nullptr)
                    delete[] end_key;
                return;
            }
        }
        if (begin_key != nullptr)
            delete[] begin_key;
        if (end_key != nullptr)
            delete[] end_key;
    }
    else
    {
        std::vector<Token> value;
        ++result_.count;
        value.push_back(Token(kNum, std::to_string(result_.count), result_.count));
        if (result_.page_id == -1)
        {
            result_.value_size_vector.clear();
            result_.total_size = 0;
            result_.value_size_vector.push_back(kSizeOfSizeT);
            result_.total_size += kSizeOfBool + kSizeOfSizeT;
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
            PageSchema result_page_schema(true, 0, -1, -1, kSizeOfSizeT + kSizeOfBool, 0, result_.total_size);
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
    }
}

void GDBE::execDelete(const Node &delete_node)
{
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
    if (database_schema_.table_schema_map[table_name].index_schema_map.find(index_name) != database_schema_.table_schema_map[table_name].index_schema_map.end())
        throw Error(kDuplicateIndexError, index_name);
    if (!database_schema_.table_schema_map[table_name].column_schema_map[column_name].index_name.empty())
        throw Error(kIndexExistError, database_schema_.table_schema_map[table_name].column_schema_map[column_name].index_name);
    TableSchema &table_schema = database_schema_.table_schema_map[table_name];
    ColumnSchema &column_schema = table_schema.column_schema_map[column_name];
    int data_type = column_schema.data_type;
    size_t key_size = data_type == 0 ? kSizeOfLong + kSizeOfBool : data_type + kSizeOfBool;
    PageSchema index_page_schema(true, 0, -1, -1, key_size, 0, kSizeOfBool + kSizeOfSizeT);
    IndexSchema index_schema;
    index_schema.column_name = column_name;
    index_schema.root_page_id = createNewPage(index_page_schema);

    auto &&iterator = BPlusTreeSelect(table_schema.root_page_id, nullptr, nullptr, false);
    auto &&begin_iter = iterator.begin();
    auto &&end_iter = iterator.end();
    bool null = false;
    size_t root_page_id = -1;
    for (auto &&iter = begin_iter; iter != end_iter; ++iter)
    {
        std::unordered_map<std::string, Token> column_token_map = toTokenMap(*iter, table_schema, kSizeOfSizeT);
        size_t page_id = iter.getPageId();
        Token key_token = column_token_map[column_name];
        char *key_ptr = new char[key_size];
        char *values_ptr = new char[kSizeOfSizeT + kSizeOfBool];
        serilization({key_token}, {key_size - kSizeOfBool}, key_ptr);
        std::copy(reinterpret_cast<const char *>(&null), reinterpret_cast<const char *>(&null) + kSizeOfBool, values_ptr);
        std::copy(reinterpret_cast<const char *>(&page_id), reinterpret_cast<const char *>(&page_id) + kSizeOfSizeT, values_ptr + kSizeOfBool);
        BPlusTreeInsert(index_schema.root_page_id, key_ptr, values_ptr, false, &root_page_id);
        if (root_page_id != -1)
        {
            index_schema.root_page_id = root_page_id;
            root_page_id = -1;
        }
        delete[] key_ptr;
        delete[] values_ptr;
    }
    column_schema.index_name = index_name;
    table_schema.index_schema_map[index_name] = index_schema;
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
    for (const auto &i : database_schema_.table_schema_map[table_name].index_schema_map)
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
    auto index_iter = table_iter->second.index_schema_map.find(index_name);
    if (index_iter == table_iter->second.index_schema_map.end())
        throw Error(kIndexNotExistError, index_name);
    size_t page_id = index_iter->second.root_page_id;
    removeBPlusTree(page_id);
    std::string column_name = index_iter->second.column_name;
    database_schema_.table_schema_map[table_name].column_schema_map[column_name].index_name = "";
    database_schema_.table_schema_map[table_name].index_schema_map.erase(index_name);
    updateDatabaseSchema();
    result_.type = kDropIndexResult;
}