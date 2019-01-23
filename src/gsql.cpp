#include "gsql.h"

void Gsql::run()
{
    std::string input;
    SyntaxTree syntax_tree;

    shell.GetInput(&input);

    front_end.process(input,&syntax_tree);

    back_end.exec(syntax_tree);
}
