#ifndef HUFFMANTREE_H
#define HUFFMANTREE_H
#include <string>

struct Node
{
    std::string letter;
    int value;
    Node *left;
    Node *right;
    Node(std::string _letter, int _value) : letter(_letter), value(_value), left(nullptr), right(nullptr) {}
};

class BinaryTree
{
private:
    Node *parent;

public:
    BinaryTree();
    void insert(std::string letter, int value);
};

#endif