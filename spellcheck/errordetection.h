#pragma once

#include <vector>
#include <QString>
#include "spellcheck.h"
#include "rbtree.h"

using namespace std;

enum ErrorType {invalidWord};

class Dictionary {
public:
	RedBlackTree<string> words;
	string path;

	Dictionary();
	Dictionary(string path);
	Dictionary(const Dictionary &d);
};

extern vector<Dictionary> dictionaries;
extern int currentDictionary;

class Error {
public:
	ErrorType type;
	int startIndex, endIndex;
	QString text;
	vector<QString> suggestions;

	pair<QString, QString> getStr();

	Error(ErrorType type, int startIndex, int endIndex, QString text) {
		this->type = type;
		this->startIndex = startIndex;
		this->endIndex = endIndex;
		this->text = text;
	}
};