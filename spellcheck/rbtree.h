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
        T key;
        Node* left, *right, *parent;
        bool color;

        Node* insert(T);
        Node* find(T);
        Node* minimum();
        Node* maximum();
        void clear();
        void save(ofstream&);
        void keys(vector<T>&);
        void keysBetween(vector<T>&, T&, T&);

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
    vector<T> keys();
    vector<T> keysBetween(T, T);
    vector<T> closestMatches(T);

    RedBlackTree();
    RedBlackTree(rbt&);
    RedBlackTree(T);
    RedBlackTree(vector<T>);
    RedBlackTree(ifstream&);
    ~RedBlackTree();
};
