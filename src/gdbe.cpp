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
    if (!file_system.exists(kDatabaseDir))
        file_system.create_directory(kDatabaseDir);
    if (file_system.exists(kDatabaseDir + string_node.token.str))
        throw Error(kDatabaseExistError, string_node.token.str);
    DatabaseSchema new_database_schema;
    new_database_schema.page_vector.push_back(0);
    std::vector<PagePtr> page_ptr_vector = new_database_schema.toPage();
    std::fstream file(kDatabaseDir + string_node.token.str, std::fstream::out);
    file_system.swap(file);
    for (size_t i = 0; i < page_ptr_vector.size(); ++i)
        file_system.write(i, page_ptr_vector[i]);
    file_system.swap(file);
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
        if (!file_system.exists(kDatabaseDir + string_node.token.str))
            throw Error(kDatabaseNotExistError, string_node.token.str);
        file_system.setFile(kDatabaseDir + string_node.token.str);
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
    if (!file_system.exists(kDatabaseDir + string_node.token.str))
        throw Error(kDatabaseNotExistError, string_node.token.str);
    if (file_system.getFilename() == kDatabaseDir + string_node.token.str)
        file_system.setFile("");
    if (database_name_ == string_node.token.str)
        database_schema_.clear();
    file_system.remove(kDatabaseDir + string_node.token.str);
}

void GDBE::execCreateTable(const Node& table_node)
{
    
}