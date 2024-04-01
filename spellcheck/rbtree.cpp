#include <iostream>
#include <fstream>
#include <vector>
#include <typeinfo>
#include <QDebug>
#include <string>
#include "rbtree.h"

using namespace std;

template class RedBlackTree<short>;
template class RedBlackTree<int>;
template class RedBlackTree<long>;
template class RedBlackTree<long long>;
template class RedBlackTree<float>;
template class RedBlackTree<double>;
template class RedBlackTree<long double>;
template class RedBlackTree<char>;
template class RedBlackTree<string>;

template<typename T> rbt::Node::Node() : Node(T(), nullptr, nullptr, nullptr, black) {}

template<typename T> rbt::Node::Node(T key) : Node(key, nullptr, nullptr, nullptr, black) {}

template<typename T> rbt::Node::Node(T key, Node* parent) : Node(key, parent, nullptr, nullptr, black) {}

template<typename T> rbt::Node::Node(T key, Node* parent, Node* left, Node* right) : Node(key, parent, left, right, black) {}

template<typename T> rbt::Node::Node(T key, Node* parent, Node* left, Node* right, bool color) {
    this->color = color;
    this->key = key;
    this->parent = parent;
    this->left = left;
    this->right = right;
}

template<typename T> rbt::Node::~Node() {
    clear();
}

template<typename T> rbt::RedBlackTree() {
    qDebug() << "rbt constructor";
    this->root = new Node();
    this->elements = 0;
}

template<typename T> rbt::RedBlackTree(T key) {
    qDebug() << "rbt constructor";
    this->root = new Node(key);
    this->elements = 1;
}

template<typename T> rbt::RedBlackTree(vector<T> keys) : RedBlackTree() {
    insert(keys);
}

template<typename T> rbt::RedBlackTree(ifstream &file) : RedBlackTree() {
    T input;
    while(file >> input) {
        insert(input);
    }
}

template<typename T> rbt::RedBlackTree(rbt &tree) : RedBlackTree() {
    ofstream file(".temp.txt");
    tree.save(file);
    file.close();
    ifstream file2(".temp.txt");
    load(file2);
    file2.close();
}

template<typename T> rbt::~RedBlackTree() {
    qDebug() << "rbt destructor";
    //clear();
    //delete this->root;
}

// forgatunk egy csomópontot a megadott irányba
// https://youtu.be/95s3ndZRGbk?si=JaxzyaHG_1hcYr0B
template<typename T> void rbt::rotate(Node* node, bool direction) {
    if(direction == dir::left) {
        Node* right = node->right;
        node->right = right->left;

        if(node->right != nullptr) node->right->parent = node;
        right->parent = node->parent;

        if(node->parent == nullptr) {
            this->root = right;
        } else if(node == node->parent->left) {
            node->parent->left = right;
        } else {
            node->parent->right = right;
        }

        right->left = node;
        node->parent = right;
        return;
    }
    Node* left = node->left;
    node->left = left->right;

    if(node->left != nullptr) node->left->parent = node;
    left->parent = node->parent;

    if(node->parent == nullptr) {
        this->root = left;
    } else if(node == node->parent->left) {
        node->parent->left = left;
    } else {
        node->parent->right = left;
    }

    left->right = node;
    node->parent = left;
}

// beszúrja a megadott értékű csomópontot
template<typename T> typename rbt::Node* rbt::Node::insert(T key) {
    if(key < this->key) {
        if(this->left == nullptr) {
            return this->left = new Node(key, this);
        }
        return this->left->insert(key);
    } else if(key > this->key) {
        if(this->right == nullptr) {
            return this->right = new Node(key, this);
        }
        return this->right->insert(key);
    }
    return nullptr;
}

// kiegyenlíti a fát buszúrás után
// https://www.youtube.com/watch?v=5IBxA-bZZH8
template<typename T> void rbt::insertFixup(Node* node) {
    #define p node->parent
    #define gp p->parent

    while(node != root && p->color == red) {
        if(p == gp->left) {
            Node* uncle = gp->right;
            if(uncle != nullptr && uncle->color == red) {
                p->color = black;
                uncle->color = black;
                gp->color = red;
                node = gp;
            } else {
                if(node == p->right) {
                    node = p;
                    rotate(node, dir::left);
                }
                p->color = black;
                gp->color = red;
                rotate(gp, dir::right);
            }
        } else {
            Node* uncle = gp->left;
            if(uncle != nullptr && uncle->color == red) {
                p->color = black;
                uncle->color = black;
                gp->color = red;
                node = gp;
            } else {
                if (node == p->left) {
                    node = p;
                    rotate(node, dir::right);
                }
                p->color = black;
                gp->color = red;
                rotate(gp, dir::left);
            }
        }
    }
    root->color = black;

    #undef gp
    #undef p
}

// beszúrja az értéket és kiegyenlíti a fát
template<typename T> rbt* rbt::insert(T key) {
    if(contains(key)) return this;
    if(this->elements == 0) {
        this->elements++;
        this->root = new Node(key);
    } else {
        Node* node = this->root->insert(key);
        node->color = red;
        if(node != nullptr) {
            this->elements++;
            insertFixup(node);
        }
    }
    return this;
}

// beszúr minden értéket a vektorból
template<typename T> rbt* rbt::insert(vector<T> keys) {
    for(T &key : keys) {
        insert(key);
    }
    return this;
}

// visszatéríti a megadott kulcsú csomópontra mutató pointert (a this részfában)
// ha nincs ilyen, akkor nullptr-t
template<typename T> typename rbt::Node* rbt::Node::find(T key) {
    if(key == this->key) {
        return this;
    } else if(key < this->key) {
        if(this->left == nullptr) return nullptr;
        return this->left->find(key);
    } else {
        if(this->right == nullptr) return nullptr;
        return this->right->find(key);
    }
}

// visszatéríti a megadott kulcsú csomópontra mutató pointert (az egész fában)
// ha nincs ilyen, akkor nullptr-t
template<typename T> typename rbt::Node* rbt::find(T key) {
    return this->root->find(key);
}

// benne van-e a megadott kulcs a fában
template<typename T> bool rbt::contains(T key) {
    return find(key) != nullptr;
}

// segít áthelyezni a részfákat a fán belül
// https://www.youtube.com/watch?v=lU99loSvD8s&t=18s
template<typename T> void rbt::transplant(Node* u, Node* v) {
    if(u->parent == nullptr) {
        this->root = v;
    } else if(u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if(v != nullptr) {
        v->parent = u->parent;
    }
}

// visszatéríti a legkisebb értékű csomópontra mutató pointert
template<typename T> typename rbt::Node* rbt::Node::minimum() {
    if(this->left == nullptr) return this;
    return this->left->minimum();
}

// visszatéríti a legnagyobb értékű csomópontra mutató pointert
template<typename T> typename rbt::Node* rbt::Node::maximum() {
    if(this->right == nullptr) return this;
    return this->right->maximum();
}

// visszatéríti a legkisebb értéket a fában
template<typename T> T rbt::minimum() {
    if(isEmpty()) throw EmptyException();
    return this->root->minimum()->key;
}

// visszatéríti a legnagyobb értéket a fában
template<typename T> T rbt::maximum() {
    if(isEmpty()) throw EmptyException();
    return this->root->maximum()->key;
}

// kiegyenlíti a fát a törlés után
template<typename T> void rbt::removeFixup(Node* node) {
    #define p node->parent

    while(node != this->root && node->color == black) {
        if(node == p->left) {
            Node* sibling = p->right;
            // 1. eset: a testvér piros
            if(sibling->color == red) { 
                sibling->color = black;
                p->color = red;
                rotate(p, dir::left);
                sibling = p->right;
            }
            // 2. eset: a testvér fekete és mindkét gyereke fekete
            if(sibling->left->color == black && sibling->right->color == black) {
                sibling->color = red;
                node = p;
            } else {
                // 3. eset: a testvér jobb gyereke fekete
                if(sibling->right->color == black) {
                    sibling->left->color = black;
                    sibling->color = red;
                    rotate(sibling, dir::right);
                    sibling = p->right;
                }
                // 4. eset: a testvér jobb gyereke piros
                sibling->color = p->color;
                p->color = sibling->right->color = black;
                rotate(p, dir::left);
                node = this->root;
            }
        } else {
            // ugyanez fordítva
            Node* sibling = p->left;
            if(sibling->color == red) {
                sibling->color = black;
                p->color = red;
                rotate(p, dir::right);
                sibling = p->left;
            }
            if(sibling->right->color == black && sibling->left->color == black) {
                sibling->color = red;
                node = p;
            } else {
                if(sibling->left->color == black) {
                    sibling->right->color = black;
                    sibling->color = red;
                    rotate(sibling, dir::left);
                    sibling = p->left;
                }
                sibling->color = p->color;
                p->color = sibling->left->color = black;
                rotate(p, dir::right);
                node = this->root;
            }
        }
    }
    node->color = black;
    #undef p
}

// törli a megadott kulcsú csomópontot és kiegyenlíti a fát
// https://www.youtube.com/watch?v=lU99loSvD8s&t=18s
template<typename T> rbt* rbt::remove(T key) {
    Node* node = find(key);
    if(node == nullptr) return this;

    bool originalColor = node->color;
    Node* child;

    if(node->left == nullptr) {
        child = node->right;
        transplant(node, node->right);
    } else if(node->right == nullptr) {
        child = node->left;
        transplant(node, node->left);
    } else {
        Node* minNode = node->right->minimum();
        child = minNode->right;
        originalColor = minNode->color;
        if(minNode->parent == node) {
            if (child != nullptr) {
                child->parent = minNode;
            }
        } else {
            transplant(minNode, minNode->right);
            minNode->right = node->right;
            minNode->right->parent = minNode;
        }
        transplant(node, minNode);
        minNode->left = node->left;
        minNode->left->parent = minNode;
        minNode->color = node->color;
    }

    if(originalColor == black) {
        removeFixup(child);
    }
    this->elements--;

    delete node;
    return this;
}

// törli az összes megadott kulcsú csomópontot
template<typename T> rbt* rbt::remove(vector<T> keys) {
    for(T &key : keys) {
        remove(key);
    }
    return this;
}

// törli a this részfát
template<typename T> void rbt::Node::clear() {
    if(this == nullptr) return;
    if(this->left != nullptr) {
        this->left->clear();
        delete this->left;
        this->left = nullptr;
    }
    if(this->right != nullptr) {
        this->right->clear();
        delete this->right;
        this->right = nullptr;
    }
}


// törli az összes csomópontot
template<typename T> rbt* rbt::clear() {
    this->root->clear();
    this->elements = 0;
    this->root = new Node();
    return this;
}

// visszatéríti hány elem van a fában
template<typename T> int rbt::size() {
    return this->elements;
}

// visszatéríti, hogy üres-e a fa
template<typename T> bool rbt::isEmpty() {
    return size() == 0;
}

// visszatéríti, hogy nem üres-e a fa
template<typename T> bool rbt::isNotEmpty() {
    return size() != 0;
}

// elmenti a this részfát a megadott fájlba úgy, hogy a betöltésnél
// ne kelljen újra kiegyenlíteni a fát 
// (preorder bejárással)
template<typename T> void rbt::Node::save(ofstream &file) {
    if(this->parent == nullptr) {
        file << this->key << " " << this->color << "\n";
    } else {
        file << this->key << " " << this->parent->key << " " << this->color << "\n";
    }
    if(this->left != nullptr) this->left->save(file);
    if(this->right != nullptr) this->right->save(file);
}

// elmenti a fát a megadott fájlba úgy, hogy a betöltésnél
// ne kelljen újra kiegyenlíteni a fát 
// (preorder bejárással)
template<typename T> rbt* rbt::save(ofstream &file) {
    if(isNotEmpty()) this->root->save(file);
    return this;
}

// betölti a fát a save metódussal elmentett fájlból
template<typename T> rbt* rbt::load(ifstream &file) {
    qDebug() << "calling load";
    T key, parentKey;
    bool color;
    if(this->elements != 0) clear();
    // ha üres a fájl, akkor nem csinál semmit
    if(!(file >> key >> color)) return this;
    // beállítja a gyökér csomópontot
    int nodes = 1;
    this->root->key = key;
    this->root->color = color;
    
    // minden csomópontra
    int test = 0;
    while(file >> key >> parentKey >> color) {
        //qDebug() << test++;
        Node* parent = find(parentKey); // megkeresi a szülőt
        Node* newNode = new Node(key, parent); // létrehozza az új csomópontot és a szülőhöz rendeli
        newNode->color = color;

        // az új csomópontot a szülő bal vagy jobb gyerekévé teszi
        if(parentKey > key) {
            parent->left = newNode;
        } else {
            parent->right = newNode;
        }
        nodes++;
    }
    this->elements = nodes;
    qDebug() << "loaded";
    return this;
}

// belerakja arr-ba a this részfa kulcsait növekvő sorrenden (inorder bejárás)
template<typename T> void rbt::Node::keys(vector<T>& arr) {
    if(this->left != nullptr) this->left->keys(arr);
    arr.push_back(this->key);
    if(this->right != nullptr) this->right->keys(arr);
}

// visszatéríti a fában lévő összes kulcsot növekvő sorrendben
template<typename T> vector<T> rbt::keys() {
    if(isEmpty()) return {};
    vector<T> keys;
    this->root->keys(keys);
    return keys;
}

// belerakja arr-ba a this részfa kulcsait, amelyek a minimum és maximum között vannak
// növekvő sorrenden (inorder bejárás)
template<typename T> void rbt::Node::keysBetween(vector<T>& arr, T &minimum, T &maximum) {
    if(this->left != nullptr) this->left->keysBetween(arr, minimum, maximum);
    if(this->key >= minimum && this->key <= maximum) arr.push_back(this->key);
    if(this->right != nullptr) this->right->keysBetween(arr, minimum, maximum);
}

// visszatéríti növekvő sorrendben a fában lévő összes kulcsot, 
// amelyek a minimum és maximum között vannak
template<typename T> vector<T> rbt::keysBetween(T minimum, T maximum) {
    if(isEmpty()) return {};
    if(minimum > maximum) swap(minimum, maximum);
    vector<T> keys;
    this->root->keysBetween(keys, minimum, maximum);
    return keys;
}

const int minClosestMatchesSize = 3;

// visszatéríti a legközelebbi értékeket a megadotthoz
template<> vector<string> RedBlackTree<string>::closestMatches(string key) {
    if(size() <= minClosestMatchesSize) return keys();
    vector<string> keys;
    string minimum = key, maximum = key;
    int i = key.length() - 1;

    // amíg nincs legalább 3 elem minimum és maximum között
    // csökkenti a minimumot és növeli a maximumot
    while(keys.size() < minClosestMatchesSize && i != 0) {
        // az i.-ik karaktert átállítja 0-re és 255-re
        // így a 0 - i-1. karakterek közti kulcsokat keressük
        minimum[i] = 0;
        maximum[i] = 255;
        keys = keysBetween(minimum, maximum);
        i--;
    }
    if(keys.size() <= minClosestMatchesSize) return keys;
    vector<string> newKeys;
    int sizeDifference = 0;
    // ha több mint 3 kulcsot kaptunk, azokat térítjük vissza
    // amelyeknek a hossza a legközelebb van a keresett kulcshoz
    while(newKeys.size() < minClosestMatchesSize) {
        for(string &str : keys) {
            if(abs((int)str.length() - (int)key.length()) == sizeDifference) {
                newKeys.push_back(str);
            }
        }
        sizeDifference++;
    }
    return newKeys;
}

// visszatéríti a legközelebbi értékeket a megadotthoz
template<typename T> vector<T> rbt::closestMatches(T key) {
    if(size() <= minClosestMatchesSize) return keys();
    vector<T> keys;
    T minimum = key, maximum = key;
    T i = static_cast<T>(0);

    // amíg nincs legalább 3 elem minimum és maximum között
    // csökkenti a minimumot és növeli a maximumot
    while(keys.size() < minClosestMatchesSize) {
        minimum -= i;
        maximum += i;
        i++;
        keys = keysBetween(minimum, maximum);
    }
    return keys;
}

