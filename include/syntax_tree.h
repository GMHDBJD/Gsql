#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include "token.h"
#include <vector>
#include <memory>

struct Node
{
  Token token;
  std::vector<Node> children;
  Node(const Token &t = kNone) : token(t) {}
};

struct SyntaxTree
{
public:
  Node *insert(Node new_node, Node *parent)
  {
    if (parent)
    {
      parent->children.push_back(new_node);
      return &parent->children.back();
    }
    else
    {
      root_ = new_node;
      return &root_;
    }
  }
  void clear()
  {
    root_ = Node(Token(kNone));
  };

  Node root_;
};

#endif
