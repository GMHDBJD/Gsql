#include "back_end.h"
#include <utility>

int BackEnd::Start(SyntaxTree&& syntax_tree)
{
    syntax_tree_ = std::move(syntax_tree);

    query_optimizer_.Optimizer(&syntax_tree_);

    gdbe_.Exec(syntax_tree_);

    return 0;
}
