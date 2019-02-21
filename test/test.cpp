#include "unit_test.h"

int main()
{
    unittest::Test test;
    test.lexerTest();
    test.parserTest();
    return 0;
}