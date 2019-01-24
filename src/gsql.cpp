#include "gsql.h"
#include <utility>

void Gsql::Run()
{
    std::string input;
    SyntaxTree syntax_tree;

    shell_.GetInput(&input);

    front_end_.Process(input,&syntax_tree);

    back_end_.Start(std::move(syntax_tree));
}
