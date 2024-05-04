﻿#include <iostream>
#include <chrono>
#include <QMutex>
#include "errordetection.h"
#include "utils.h"

using namespace std;

typedef chrono::time_point<chrono::high_resolution_clock> TimePoint;
typedef chrono::duration<double> Duration;

vector<Dictionary> dictionaries;
int currentDictionary;
QString autoDetectedDictionary = "";
int autoDetectedDictionaryID = -1;
const int autoDetect = -1;
const QString validChars = "abcdefghijklmnopqrstuvwxyzüóőúűéáăîâșț";

int getCurrentDictionary() {
	return currentDictionary == autoDetect ? autoDetectedDictionaryID : currentDictionary;
}

Error::Error(ErrorType type = ErrorType::none, int startIndex = 0, int endIndex = 0, QString text = "") {
	this->type = type;
	this->startIndex = startIndex;
	this->endIndex = endIndex;
	this->text = text;
}

void Error::getSuggestions(int dict) {
	TimePoint start = chrono::high_resolution_clock::now();
	this->suggestions.clear();
	if(dict == -1) dict = getCurrentDictionary();
	if(type == invalidWord && dictionaries.size() > dict) {
		if(this->text.size() < 2) return;

		TimePoint start = chrono::high_resolution_clock::now();
		dictionaries[dict].words.closestMatches(this->text.toLower().toStdString(), 5, 500, this->suggestions);
		Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
		qDebug() << "getting closest matches took " << elapsedSeconds.count() << " seconds";

		// kicseréljük egyesével a betűket, ha az valid betesszük
		for(int i = 0; i < this->text.size(); i++) {
			QString word = this->text.normalized(QString::NormalizationForm_KC);
			for(const QChar &c : validChars) {
				if(word[i] == c) continue;
				QString modifiedWord = word;
				modifiedWord[i] = c;
				if(dictionaries[dict].words.contains(modifiedWord.toStdString())) this->suggestions.push_back(modifiedWord.toStdString());
			}
		}

		// felcseréljük a szomszédos betűket, ha az valid betesszük
		for(int i = 0; i < this->text.size() - 1; i++) {
			QString word = this->text;
			swap(word[i], word[i + 1]);
			if(dictionaries[dict].words.contains(word.toLower().toStdString())) this->suggestions.push_back(word.toStdString());
		}

		// töröljük a betűket, ha az valid betesszük
		for(int i = 0; i < this->text.size(); i++) {
			QString word = this->text;
			word.remove(i, 1);
			if(dictionaries[dict].words.contains(word.toLower().toStdString())) this->suggestions.push_back(word.toStdString());
		}

		// ha a szó kettéválasztható két valid szóvá szedjük szét
		for(int i = 0; i < this->text.size() - 1; i++) {
			QString word1 = this->text.left(i + 1);
			QString word2 = this->text.right(this->text.size() - i - 1);
			if(dictionaries[dict].words.contains(word1.toLower().toStdString()) && dictionaries[dict].words.contains(word2.toLower().toStdString())) {
				this->suggestions.push_back(word1.toStdString() + " " + word2.toStdString());
			}
		}

		removeDuplicates(this->suggestions);
		sortByRelevance(this->suggestions, this->text.toStdString());

		for(string &suggestion : this->suggestions) {
			suggestion = maintainCase(suggestion, this->text.toStdString());			
		}
	}

	if(this->suggestions.size() > 5) this->suggestions.resize(5);
	
	Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
	qDebug() << "getting suggestions took " << elapsedSeconds.count() << " seconds";
}

Error getErrorAt(int index, QString text, vector<Error> &errors) {
	for(Error &error : errors) {
		if(index >= error.startIndex && index <= error.endIndex) return error;
	}
	return Error(none, -1, -1, "");
}

Dictionary::Dictionary() : words() {
	this->path = "";
	this->changed = false;
}

Dictionary::Dictionary(string path) : words() {
	this->path = path;
	this->changed = false;
	ifstream file("data/dictionaries/" + path);
	if(!file.good()) {
		qDebug() << "could not open file " << path;
		return;
	}
	words.load(file);
	file.close();
	qDebug() << "Dictionary loaded: " << path;
}

Dictionary::Dictionary(ifstream &file, QString fileName) : Dictionary()  {
    if(!file.good()) {
        qDebug() << "could not open file ";
        return;
    }
    this->words = RedBlackTree<string>(file);
	this->changed = false;
	qDebug() << "Dictionary loaded: " << this->words.size() << " words";
	string path = "data/dictionaries/" + fileName.toStdString() + ".dict";
	ofstream out(path);
	this->words.save(out);
	this->path = (fileName + ".dict").toStdString();
}

Dictionary::Dictionary(const Dictionary &d) {
	qDebug() << "Dictionary copy constructor";
	this->words = d.words;
	this->path = d.path;
}

pair<QString, QString> Error::getStr() {
	switch (this->type) {
		case invalidWord: return {"Spelling", "Did you mean to write: "};
		default: return {"", ""};
	}
}

bool isSeparator(QChar c) {
	QString separators = " \n\t.,?!()[]{}:;\"'`";
	return separators.contains(c);
}

void FileTab::detectErrors(QString text) {
	TimePoint start = chrono::high_resolution_clock::now();
	this->errors.clear();
	text += " ";

	int dict = getCurrentDictionary();

	// helytelen szó ellenőrzés
	if(dictionaries.size() > dict) {
		QString word = "";
		for(int i = 0; i < text.size(); i++) {
			if(isSeparator(text[i])) {
				if(word.length() == 0) continue;
				bool isValid = dictionaries[dict].words.contains(word.toLower().toStdString());
				if(!isValid) this->errors.push_back(Error(invalidWord, i - word.length(), i, word));
				word = "";
			} else {
				word += text[i];
			}
		}
	}
	
	Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
	qDebug() << "Error detection took " << elapsedSeconds.count() << " seconds";
	this->errorDetectionTime = elapsedSeconds.count() * 1000;
}

//static QMutex mutex;

void spellcheck::underlineErrors() {
	TimePoint start = chrono::high_resolution_clock::now();
	if(textEdit == nullptr) return;	
	QTextCursor cursor = textEdit->textCursor();

    // töroljuk az előző hibákat
    cursor.select(QTextCursor::Document);
    textEdit->blockSignals(true);
    cursor.setCharFormat(QTextCharFormat());
    textEdit->blockSignals(false);
    cursor.clearSelection();
	
	if(focusedFile->errors.size() > 100) return; 

	// aláhúzzuk a hibákat
	QString text = textEdit->toPlainText();
	textEdit->blockSignals(true);
	QTextCharFormat format;
	format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	format.setUnderlineColor(QColor(style::accentColor));
	//for(Error &error : focusedFile->errors) {
	for(int i = 0; i < focusedFile->errors.size(); i++) {
		Error &error = focusedFile->errors[i];
		cursor.setPosition(error.startIndex);
		cursor.setPosition(error.endIndex, QTextCursor::KeepAnchor);
		cursor.setCharFormat(format);
	}
	textEdit->blockSignals(false);

	Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
	qDebug() << "Underlining errors took " << elapsedSeconds.count() << " seconds";
}

void spellcheck::underlineErrorsLater() {
	static bool isFirstCall = true;
	static TimePoint lastCall = chrono::high_resolution_clock::now();
	const int interval = 5000;
	if(!isFirstCall && chrono::high_resolution_clock::now() - lastCall < chrono::milliseconds(interval)) return;
	lastCall = chrono::high_resolution_clock::now();
	isFirstCall = false;
	QTimer::singleShot(interval, this, [=] {
		underlineErrors();
	});
}