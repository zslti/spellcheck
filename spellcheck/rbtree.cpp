#include <iostream>
#include <fstream>
#include <vector>
#include <typeinfo>
#include <QDebug>
#include <string>
#include <stack>
#include "rbtree.h"
#include "utils.h"

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

template<typename T> rbt::Node::Node(T value) : Node(value, nullptr, nullptr, nullptr, black) {}

template<typename T> rbt::Node::Node(T value, Node* parent) : Node(value, parent, nullptr, nullptr, black) {}

template<typename T> rbt::Node::Node(T value, Node* parent, Node* left, Node* right) : Node(value, parent, left, right, black) {}

template<typename T> rbt::Node::Node(T value, Node* parent, Node* left, Node* right, bool color) {
    this->color = color;
    this->value = value;
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

template<typename T> rbt::RedBlackTree(T value) {
    qDebug() << "rbt constructor";
    this->root = new Node(value);
    this->elements = 1;
}

template<typename T> rbt::RedBlackTree(vector<T> values) : RedBlackTree() {
    insert(values);
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
template<typename T> typename rbt::Node* rbt::Node::insert(T value) {
    if(value < this->value) {
        if(this->left == nullptr) {
            return this->left = new Node(value, this);
        }
        return this->left->insert(value);
    } else if(value > this->value) {
        if(this->right == nullptr) {
            return this->right = new Node(value, this);
        }
        return this->right->insert(value);
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
template<typename T> rbt* rbt::insert(T value) {
    if(contains(value)) return this;
    if(this->elements == 0) {
        this->elements++;
        this->root = new Node(value);
    } else {
        Node* node = this->root->insert(value);
        node->color = red;
        if(node != nullptr) {
            this->elements++;
            insertFixup(node);
        }
    }
    return this;
}

// beszúr minden értéket a vektorból
template<typename T> rbt* rbt::insert(vector<T> values) {
    for(T &value : values) {
        insert(value);
    }
    return this;
}

// visszatéríti a megadott kulcsú csomópontra mutató pointert (a this részfában)
// ha nincs ilyen, akkor nullptr-t
template<typename T> typename rbt::Node* rbt::Node::find(T value) {
    if(value == this->value) {
        return this;
    } else if(value < this->value) {
        if(this->left == nullptr) return nullptr;
        return this->left->find(value);
    } else {
        if(this->right == nullptr) return nullptr;
        return this->right->find(value);
    }
}

// visszatéríti a megadott kulcsú csomópontra mutató pointert (az egész fában)
// ha nincs ilyen, akkor nullptr-t
template<typename T> typename rbt::Node* rbt::find(T value) {
    return this->root->find(value);
}

// benne van-e a megadott kulcs a fában
template<typename T> bool rbt::contains(T value) {
    return find(value) != nullptr;
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
    return this->root->minimum()->value;
}

// visszatéríti a legnagyobb értéket a fában
template<typename T> T rbt::maximum() {
    if(isEmpty()) throw EmptyException();
    return this->root->maximum()->value;
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
template<typename T> rbt* rbt::remove(T value) {
    Node* node = find(value);
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
template<typename T> rbt* rbt::remove(vector<T> values) {
    for(T &value : values) {
        remove(value);
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
        file << this->value << " " << this->color << "\n";
    } else {
        file << this->value << " " << this->parent->value << " " << this->color << "\n";
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
    T value, parentValue;
    bool color;
    if(this->elements != 0) clear();
    // ha üres a fájl, akkor nem csinál semmit
    if(!(file >> value >> color)) return this;
    // beállítja a gyökér csomópontot
    int nodes = 1;
    this->root->value = value;
    this->root->color = color;
    
    // minden csomópontra
    while(file >> value >> parentValue >> color) {
        Node* parent = find(parentValue); // megkeresi a szülőt
        Node* newNode = new Node(value, parent); // létrehozza az új csomópontot és a szülőhöz rendeli
        newNode->color = color;

        // az új csomópontot a szülő bal vagy jobb gyerekévé teszi
        if(parentValue > value) {
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
template<typename T> void rbt::Node::values(vector<T> &arr) {
    if(this->left != nullptr) this->left->values(arr);
    arr.push_back(this->value);
    if(this->right != nullptr) this->right->values(arr);
}

// visszatéríti a fában lévő összes kulcsot növekvő sorrendben
template<typename T> vector<T> rbt::values() {
    if(isEmpty()) return {};
    vector<T> values;
    this->root->values(values);
    return values;
}

// belerakja arr-ba a this részfa kulcsait, amelyek a minimum és maximum között vannak
// növekvő sorrenden (inorder bejárás)
template<typename T> void rbt::Node::valuesBetween(vector<T> &values, T &minimum, T &maximum) {
    stack<Node*> stack;
    Node* currentNode = this;

    while(currentNode != nullptr || !stack.empty()) {
        // megyünk a legbaloldali csomópontig, ami nagyobb vagy egyenlő a minimummal
        while(currentNode) {
            stack.push(currentNode);
            currentNode = currentNode->left;
        }
        currentNode = stack.top();
        stack.pop();

        if(currentNode->value >= minimum && currentNode->value <= maximum) {
            values.push_back(currentNode->value);
        }

        // ha nagyobb mint a maximum, már nem fogunk tőle jobbra kisebbet találni
        if (currentNode->value > maximum) return;

        // megyünk a jobb részfára
        currentNode = currentNode->right;
    }
}

// visszatéríti növekvő sorrendben a fában lévő összes kulcsot, 
// amelyek a minimum és maximum között vannak
template<typename T> vector<T> rbt::valuesBetween(T minimum, T maximum, int maxValues) {
    if(isEmpty()) return {};
    if(minimum > maximum) swap(minimum, maximum);
    vector<T> values;
    this->root->valuesBetween(values, minimum, maximum);
    if(values.size() > maxValues) values.resize(maxValues);
    return values;
}

template<typename T> vector<T> rbt::valuesBetween(T minimum, T maximum) {
    return valuesBetween(minimum, maximum, INT_MAX);
}

template<typename T> void rbt::valuesBetween(T minimum, T maximum, int maxValues, vector<T> &values) {
	if(isEmpty()) return;
    if(minimum > maximum) swap(minimum, maximum);
    this->root->valuesBetween(values, minimum, maximum);
    if(values.size() > maxValues) values.resize(maxValues);
}

const int minClosestMatchesSize = 3;

// visszatéríti a legközelebbi értékeket a megadotthoz
template<> vector<string> RedBlackTree<string>::closestMatches(string key, int maxTries, int maxValues) {
    vector<string> closestMatches;
    this->closestMatches(key, maxTries, maxValues, closestMatches);
    return closestMatches;
}

template<> void RedBlackTree<string>::closestMatches(string key, int maxTries, int maxValues, vector<string> &closestMatches) {
    if(size() <= minClosestMatchesSize) {
        closestMatches = values();
        return;
    }
    string minimum = key, maximum = key;
    int i = key.length() - 1;
    int t = 0;

    // amíg nincs legalább 3 elem minimum és maximum között
    // csökkenti a minimumot és növeli a maximumot
    while(closestMatches.size() < minClosestMatchesSize && i > 0) {
        // az i.-ik karaktert átállítja 0-re és 255-re
        // így a 0 - i-1. karakterek közti kulcsokat keressük
        minimum[i] = 0;
        maximum[i] = 255;
        valuesBetween(minimum, maximum, maxValues, closestMatches);
        i--;
        if(t == maxTries) break;
    }

    if(closestMatches.size() > maxValues) closestMatches.resize(maxValues);

    sortByRelevance(closestMatches, key);
}

template<typename T> vector<T> rbt::closestMatches(T key, int maxTries, int maxValues) {
    vector<T> closestMatches;
    this->closestMatches(key, maxTries, maxValues, closestMatches);
    return closestMatches;
}

// visszatéríti a legközelebbi értékeket a megadotthoz
template<typename T> void rbt::closestMatches(T key, int maxTries, int maxValues, vector<T> &closestMatches) {
    if(size() <= minClosestMatchesSize) {
        closestMatches = values();
        return;
    }
    T minimum = key, maximum = key;
    T i = static_cast<T>(0);
    int t = 0;

    // amíg nincs legalább 3 elem minimum és maximum között
    // csökkenti a minimumot és növeli a maximumot
    while(closestMatches.size() < minClosestMatchesSize) {
        minimum -= i;
        maximum += i;
        i++;
        t++;
        valuesBetween(minimum, maximum, maxValues, closestMatches);
        if(t == maxTries) break;
    }
}

template<typename T> vector<T> rbt::closestMatches(T key, int maxTries) {
    return closestMatches(key, maxTries, INT_MAX);
}

template<typename T> vector<T> rbt::closestMatches(T key) {
    return closestMatches(key, -1, INT_MAX);
}