#pragma once

#include <iostream>
#include <fstream>
#include <vector>

#define rbt RedBlackTree<T>
#define dir RedBlackTree<int>::direction

using namespace std;

template<typename T> class RedBlackTree {  
    class Node {
    public:
        T value;
        Node* left, *right, *parent;
        bool color;

        Node* insert(T);
        Node* find(T);
        Node* minimum();
        Node* maximum();
        void clear();
        void save(ofstream&);
        void values(vector<T>&);
        void valuesBetween(vector<T>&, T&, T&);

        Node();
        Node(T);
        Node(T, Node*);
        Node(T, Node*, Node*, Node*);
        Node(T, Node*, Node*, Node*, bool);
        ~Node();
    };
    Node* root;
    int elements;
    void rotate(Node*, bool);
    void insertFixup(Node*);
    void transplant(Node*, Node*);
    void removeFixup(Node*);
    Node* find(T);
public:
    enum color {black, red};
    enum direction {left, right};
    class EmptyException{};
    rbt* insert(T);
    rbt* insert(vector<T>);
    rbt* remove(T);
    rbt* remove(vector<T>);
    rbt* clear();
    bool contains(T);
    T minimum();
    T maximum();
    int size();
    bool isEmpty();
    bool isNotEmpty();
    rbt* save(ofstream&);
    rbt* load(ifstream&);
    vector<T> values();
    vector<T> valuesBetween(T, T);
    vector<T> valuesBetween(T, T, int);
    vector<T> closestMatches(T);
    vector<T> closestMatches(T, int);
    vector<T> closestMatches(T, int, int);

    RedBlackTree();
    RedBlackTree(rbt&);
    RedBlackTree(T);
    RedBlackTree(vector<T>);
    RedBlackTree(ifstream&);
    ~RedBlackTree();
};
