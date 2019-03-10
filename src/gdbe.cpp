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
    std::fstream file(kDatabaseDir + string_node.token.str, std::fstream::out);
    file_system_.swap(file);
    for (size_t i = 0; i < page_ptr_vector.size(); ++i)
        file_system_.write(i, page_ptr_vector[i]);
    file_system_.swap(file);
    file.close();
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
        database_schema_.clear();
        stream >> database_schema_.page_vector;
        std::vector<PagePtr> page_ptr_vector;
        for (auto &&i : database_schema_.page_vector)
        {
            page_ptr_vector.push_back(buffer_pool_.getPage(i));
        }
        database_schema_.pageTo(page_ptr_vector);
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
}

void GDBE::execCreateTable(const Node &table_node)
{
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
                new_column_schema.data_type = kIntType;
                break;
            case kChar:
                new_column_schema.data_type = kCharType;
                break;
            default:
                throw Error(kSyntaxTreeError, node.childern[1].token.str);
            }
            auto rc = new_table_schema.column_schema_map.insert({column_name, new_column_schema});
            if (!rc.second)
                throw Error(kDuplicateColumnError, column_name);
            if (node.childern.size() == 3)
                new_column_schema.not_null = true;
        }
    }
    for (const auto &node : columns_node.childern)
    {
        if (node.token.token_type == kReferences)
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
    new_table_schema.root_page_num = getFreePage();
    database_schema_.table_schema_map[table_name] = new_table_schema;
}

void GDBE::execShowTables(const Node &tables_node)
{
    std::vector<std::string> string_vector;
    for(const auto& i : database_schema_.table_schema_map)
    {
        string_vector.push_back(i.first);   
    }
    result_.type_ = kShowTablesResult;
    result_.string_vector_vector_.push_back(std::move(string_vector));
}