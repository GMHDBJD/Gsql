#include "parser.h"
#include <iostream>

SyntaxTree Parser::parse(std::queue<Token> token_queue)
{
    syntax_tree_.clear();
    setTokenQueue(std::move(token_queue));
    try
    {
        build(parseAll());
    }
    catch (const Error &error)
    {
        errorProcess(error);
    }
    return std::move(syntax_tree_);
}

Node Parser::parseAll()
{
    Node temp_node;
    switch (lookAhead().token_type)
    {
    case kShow:
        temp_node = parseShow();
        break;
    case kUse:
        temp_node = parseUse();
        break;
    case kCreate:
        temp_node = parseCreate();
        break;
    case kInsert:
        temp_node = parseInsert();
        break;
    case kSelect:
        temp_node = parseSelect();
        break;
    case kDelete:
        temp_node = parseDelete();
        break;
    case kDrop:
        temp_node = parseDrop();
        break;
    case kUpdate:
        temp_node = parseUpdate();
        break;
    case kAlter:
        temp_node = parseAlter();
        break;
    case kExplain:
        temp_node = parseExplain();
        break;
    case kExit:
        temp_node = Node(next());
        break;
    default:
        throw Error(kSqlError, lookAhead().str);
    }
    match(kSemicolon);
    return temp_node;
}

Node Parser::parseShow()
{
    Node show_node{match(kShow)};
    switch (lookAhead().token_type)
    {
    case kDatabases:
    case kTables:
        build(next(), &show_node);
        return show_node;
    case kIndex:
        build(next(), &show_node);
        match(kFrom);
        build(parseName(2), &show_node);
        return show_node;
    default:
        throw Error(kSqlError, lookAhead().str);
    }
}

Node Parser::parseUse()
{
    Node use_node{match(kUse)};
    Node *database_node_ptr = build(Token(kDatabase, "DATABASE"), &use_node);
    build(match(kStr), database_node_ptr);
    return use_node;
}

Node Parser::parseCreate()
{
    Node creat_node{match(kCreate)};
    switch (lookAhead().token_type)
    {
    case kDatabase:
    {
        Node *database_node_ptr = build(next(), &creat_node);
        build(match(kStr), database_node_ptr);
        return creat_node;
    }
    case kTable:
    {
        Node *table_node_ptr = build(next(), &creat_node);
        build(parseName(2), table_node_ptr);
        match(kLeftParenthesis);
        Node columns_node = parseColumns();
        build(columns_node, table_node_ptr);
        match(kRightParenthesis);
        return creat_node;
    }
    case kIndex:
    {
        Node *index_node_ptr = build(next(), &creat_node);
        build(match(kStr), index_node_ptr);
        match(kOn);
        Node *name_node_ptr = build(parseName(2), index_node_ptr);
        match(kLeftParenthesis);
        build(parseNames(3), index_node_ptr);
        match(kRightParenthesis);
        return creat_node;
    }
    default:
        throw Error(kSqlError, lookAhead().str);
    }
}

Node Parser::parseInsert()
{
    Node insert_node{match(kInsert)};
    match(kInto);
    build(parseName(2), &insert_node);

    if (lookAhead().token_type == kLeftParenthesis)
    {
        match(kLeftParenthesis);
        build(parseNames(3), &insert_node);
        match(kRightParenthesis);
    }
    Node values_node = parseValues();
    build(values_node, &insert_node);
    return insert_node;
}

Node Parser::parseSelect()
{
    Node select_node{match(kSelect)};
    Node *columns_node_ptr = build(Token(kColumns, "COLUMNS"), &select_node);
    switch (lookAhead().token_type)
    {
    case kMultiply:
    {
        build(next(), columns_node_ptr);
        if (lookAhead().token_type == kComma)
        {
            next();
            build(parseExprs(), columns_node_ptr);
        }
        break;
    }
    default:
    {
        build(parseExprs(), columns_node_ptr);
        break;
    }
    }
    match(kFrom);
    build(parseNames(2), &select_node);
    if (lookAhead().token_type == kJoin)
        build(parseJoins(), &select_node);
    if (lookAhead().token_type == kWhere)
        build(parseWhere(), &select_node);
    if (lookAhead().token_type == kLimit)
    {
        Node *limit_node_ptr = build(next(), &select_node);
        build(match(kNum), limit_node_ptr);
    }
    return select_node;
}

Node Parser::parseDelete()
{
    Node delete_node{match(kDelete)};
    match(kFrom);
    build(parseName(2), &delete_node);
    if (lookAhead().token_type == kWhere)
        build(parseWhere(), &delete_node);
    return delete_node;
}

Node Parser::parseDrop()
{
    Node drop_node{match(kDrop)};
    switch (lookAhead().token_type)
    {
    case kDatabase:
    case kTable:
    {
        Node *node_ptr = build(next(), &drop_node);
        build(parseName(2), node_ptr);
        return drop_node;
    }
    case kIndex:
    {
        Node *index_node_ptr = build(next(), &drop_node);
        build(match(kStr), index_node_ptr);
        match(kOn);
        build(parseName(2), index_node_ptr);
    }
    default:
        throw Error(kSqlError, lookAhead().str);
    }
}

Node Parser::parseUpdate()
{
    Node update_node{match(kUpdate)};
    build(parseName(2), &update_node);
    match(kSet);
    build(parseExprs(), &update_node);
    if (lookAhead().token_type == kWhere)
        build(parseWhere(), &update_node);
    return update_node;
}

Node Parser::parseAlter()
{
    Node alter_node{match(kAlter)};
    match(kTable);
    build(parseName(2), &alter_node);
    switch (lookAhead().token_type)
    {
    case kAdd:
    {
        Node *add_node_ptr = build(next(), &alter_node);
        build(parseColumn(), add_node_ptr);
        return alter_node;
    }
    case kDrop:
    {
        Node *drop_node_ptr = build(next(), &alter_node);
        match(kColumn);
        build(parseName(3), drop_node_ptr);
        return alter_node;
    }
    case kAlter:
    {
        Node *alter_node_ptr = build(next(), &alter_node);
        match(kColumn);
        build(parseColumn(), alter_node_ptr);
        return alter_node;
    }
    default:
        throw Error(kSqlError, lookAhead().str);
    }
}

Node Parser::parsePrimary()
{
    Node primary_node{match(kPrimary)};
    match(kKey);
    match(kLeftParenthesis);
    build(parseNames(3), &primary_node);
    match(kRightParenthesis);
    return primary_node;
}

Node Parser::parseColumns()
{
    Node columns_node{Token(kColumns, "COLUMNS")};
    build(parseColumn(), &columns_node);
    while (lookAhead().token_type == kComma)
    {
        next();
        switch (lookAhead().token_type)
        {
        case kPrimary:
            build(parsePrimary(), &columns_node);
            break;
        case kForeign:
            build(parseForeign(), &columns_node);
            break;
        default:
            build(parseColumn(), &columns_node);
            break;
        }
    }
    return columns_node;
}

Node Parser::parseForeign()
{
    Node foreign_node{match(kForeign)};
    match(kKey);
    match(kLeftParenthesis);
    build(parseName(3), &foreign_node);
    match(kRightParenthesis);
    match(kReferences);
    build(parseName(2), &foreign_node);
    match(kLeftParenthesis);
    build(parseName(3), &foreign_node);
    match(kRightParenthesis);
    return foreign_node;
}

Node Parser::parseColumn()
{
    Node column_node{Token(kColumn, "COLUMN")};
    build(parseName(3), &column_node);
    switch (lookAhead().token_type)
    {
    case kInt:
        build(next(), &column_node);
        break;
    case kChar:
    {
        Node *char_node_ptr = build(next(), &column_node);
        if (lookAhead().token_type == kLeftParenthesis)
        {
            next();
            build(match(kNum), char_node_ptr);
            match(kRightParenthesis);
        }
        break;
    }
    default:
        throw Error(kSqlError, lookAhead().str);
    }
    TokenType token_type;
    while ((token_type = lookAhead().token_type) == kNot || token_type == kDefault || token_type == kUnique)
    {
        switch (token_type)
        {
        case kNot:
        {
            next();
            match(kNull);
            build(Token(kNotNull, "NOTNULL"), &column_node);
            break;
        }
        case kDefault:
        {
            Node *default_node_ptr = build(next(), &column_node);
            switch (lookAhead().token_type)
            {
            case kString:
            case kNum:
                build(next(), default_node_ptr);
                break;
            default:
                throw Error(kSqlError, lookAhead().str);
            }
            break;
        }
        case kUnique:
        {
            build(next(), &column_node);
            break;
        }

        default:
            break;
        }
    }
    return column_node;
}

Node Parser::parseName(size_t count)
{
    size_t cnt = 1;
    Node name_node{Token(kName, "NAME")};
    build(match(kStr), &name_node);
    while (lookAhead().token_type == kFullStop)
    {
        next();
        ++cnt;
        if (cnt > count)
            throw Error(kSqlError, ".");
        build(match(kStr), &name_node);
    }
    return name_node;
}

Node Parser::parseNames(size_t count)
{
    Node names_node{Token(kNames, "NAMES")};
    build(parseName(count), &names_node);
    while (lookAhead().token_type == kComma)
    {
        next();
        build(parseName(3), &names_node);
    }
    return names_node;
}

Node Parser::parseValues()
{
    Node values_node{match(kValues)};
    build(parseValue(), &values_node);
    while (lookAhead().token_type == kComma)
    {
        next();
        build(parseValue(), &values_node);
    }
    return values_node;
}

Node Parser::parseValue()
{
    match(kLeftParenthesis);
    Node exprs_node = parseExprs();
    match(kRightParenthesis);
    return exprs_node;
}

Node Parser::parseExprs()
{
    Node exprs_node{Token(kExprs, "EXPRS")};
    build(parseExpr(), &exprs_node);
    while (lookAhead().token_type == kComma)
    {
        next();
        build(parseExpr(), &exprs_node);
    }
    return exprs_node;
}

Node Parser::parseExpr()
{
    Node expr_node{Token(kExpr, "EXPR")};
    build(parseOr(), &expr_node);
    return expr_node;
}

Node Parser::parseOr()
{
    Node or_node = parseAnd();
    while (lookAhead().token_type == kOr)
    {
        Node new_or_node{next()};
        build(or_node, &new_or_node);
        build(parseAnd(), &new_or_node);
        or_node = std::move(new_or_node);
    }
    return or_node;
}

Node Parser::parseAnd()
{
    Node and_node = parseBitsOr();
    while (lookAhead().token_type == kAnd)
    {
        Node new_and_node{next()};
        build(and_node, &new_and_node);
        build(parseBitsOr(), &new_and_node);
        and_node = std::move(new_and_node);
    }
    return and_node;
}

Node Parser::parseBitsOr()
{
    Node bits_or_node = parseBitsExclusiveOr();
    while (lookAhead().token_type == kBitsOr)
    {
        Node new_bits_or_node{next()};
        build(bits_or_node, &new_bits_or_node);
        build(parseBitsExclusiveOr(), &new_bits_or_node);
        bits_or_node = std::move(new_bits_or_node);
    }
    return bits_or_node;
}

Node Parser::parseBitsExclusiveOr()
{
    Node bits_exclusive_or_node = parseBitsAnd();
    while (lookAhead().token_type == kBitsExclusiveOr)
    {
        Node new_bits_exclusive_or_node{next()};
        build(bits_exclusive_or_node, &new_bits_exclusive_or_node);
        build(parseBitsAnd(), &new_bits_exclusive_or_node);
        bits_exclusive_or_node = new_bits_exclusive_or_node;
    }
    return bits_exclusive_or_node;
}

Node Parser::parseBitsAnd()
{
    Node bits_and_node = parseEqualOrNot();
    while (lookAhead().token_type == kBitsAnd)
    {
        Node new_bits_and_node{next()};
        build(bits_and_node, &new_bits_and_node);
        build(parseEqualOrNot(), &new_bits_and_node);
        bits_and_node = std::move(new_bits_and_node);
    }
    return bits_and_node;
}

Node Parser::parseEqualOrNot()
{
    Node equal_or_not_node = parseCompare();
    TokenType token_type;
    while ((token_type = lookAhead().token_type) == kEqual || token_type == kNotEqual)
    {
        Node new_equal_or_not_node{next()};
        build(equal_or_not_node, &new_equal_or_not_node);
        build(parseCompare(), &new_equal_or_not_node);
        equal_or_not_node = std::move(new_equal_or_not_node);
    }
    return equal_or_not_node;
}

Node Parser::parseCompare()
{
    Node compare_node = parseShift();
    TokenType token_type;
    while ((token_type = lookAhead().token_type) == kLess || token_type == kLessEqual || token_type == kGreater || token_type == kGreaterEqual)
    {
        Node new_compare_node{next()};
        build(compare_node, &new_compare_node);
        build(parseShift(), &new_compare_node);
        compare_node = std::move(new_compare_node);
    }
    return compare_node;
}

Node Parser::parseShift()
{
    Node shift_node = parsePlusMinus();
    TokenType token_type;
    while ((token_type = lookAhead().token_type) == kShiftLeft || token_type == kShiftRight)
    {
        Node new_shift_node{next()};
        build(shift_node, &new_shift_node);
        build(parsePlusMinus(), &new_shift_node);
        shift_node = std::move(new_shift_node);
    }
    return shift_node;
}

Node Parser::parsePlusMinus()
{
    Node plus_minus_node = parseMultiplyDivideMod();
    TokenType token_type;
    while ((token_type = lookAhead().token_type) == kPlus || token_type == kMinus)
    {
        Node new_plus_minus_node{next()};
        build(plus_minus_node, &new_plus_minus_node);
        build(parseMultiplyDivideMod(), &new_plus_minus_node);
        plus_minus_node = std::move(new_plus_minus_node);
    }
    return plus_minus_node;
}

Node Parser::parseMultiplyDivideMod()
{
    Node multiply_divide_mod_node = parseNotOrBitsNegative();
    TokenType token_type;
    while ((token_type = lookAhead().token_type) == kMultiply || token_type == kDivide || token_type == kMod)
    {
        Node new_multiply_divide_mod_node{next()};
        build(multiply_divide_mod_node, &new_multiply_divide_mod_node);
        build(parseNotOrBitsNegative(), &new_multiply_divide_mod_node);
        multiply_divide_mod_node = std::move(new_multiply_divide_mod_node);
    }
    return multiply_divide_mod_node;
}

Node Parser::parseNotOrBitsNegative()
{
    TokenType token_type = lookAhead().token_type;
    if (token_type == kNot || token_type == kBitsNot || token_type == kMinus)
    {
        Node not_or_bits_negative{next()};
        build(parseItem(), &not_or_bits_negative);
        return not_or_bits_negative;
    }
    Node item_node = parseItem();
    return item_node;
}

Node Parser::parseItem()
{
    Node item_node;
    switch (lookAhead().token_type)
    {
    case kLeftParenthesis:
    {
        next();
        item_node = parseOr();
        match(kRightParenthesis);
        return item_node;
    }
    case kNum:
    case kNull:
    case kString:
    {
        item_node = Node(next());
        return item_node;
    }
    case kStr:
    {
        item_node = parseName(3);
        return item_node;
    }
    default:
        throw Error(kSqlError, lookAhead().str);
    }
    return item_node;
}

Node Parser::parseJoins()
{
    Node joins_node{Token(kJoins, "JOINS")};
    build(parseJoin(), &joins_node);
    while (lookAhead().token_type == kJoin)
    {
        build(parseJoin());
    }
    return joins_node;
}

Node Parser::parseJoin()
{
    Node join_node{next()};
    build(parseName(2), &join_node);
    if (lookAhead().token_type == kOn)
    {
        next();
        build(parseExpr(), &join_node);
    }
    return join_node;
}

Node Parser::parseWhere()
{
    Node where_node{next()};
    build(parseExpr(), &where_node);
    return where_node;
}

Node Parser::parseExplain()
{
    Node explain_node{match(kExplain)};
    build(parseName(2), &explain_node);
    return explain_node;
}