#pragma once

#include <QRegularExpression>
#include "spellcheck.h"
#include "rbtree.h"
#include "utils.h"

using namespace std;

enum ErrorType {none, invalidWord, whitespace, capitalization, repeatedWords};

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

	vector<string>& getSuggestions(int dict = -1);
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