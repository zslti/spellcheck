#pragma once

#include <vector>
#include <QString>
#include "spellcheck.h"
#include "rbtree.h"

using namespace std;

enum ErrorType {invalidWord, none};

class Dictionary {
public:
	RedBlackTree<string> words;
	string path;
	bool changed;

	Dictionary();
	Dictionary(string path);
	Dictionary(ifstream &file, QString fileName);
	Dictionary(const Dictionary &d);
};

class Error {
public:
	ErrorType type;
	int startIndex, endIndex;
	QString text;
	vector<string> suggestions;

	pair<QString, QString> getStr();

	Error(ErrorType type, int startIndex, int endIndex, QString text);

	void getSuggestions();
};

extern vector<Dictionary> dictionaries;
extern int currentDictionary;
extern const int autoDetect;
extern int getCurrentDictionary();
extern QString autoDetectedDictionary;
extern int autoDetectedDictionaryID;
extern bool isSeparator(QChar c);
extern Error getErrorAt(int index, QString text, vector<Error> &errors);
extern const QString validChars;