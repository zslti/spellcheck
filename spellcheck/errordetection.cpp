#include <iostream>
#include <chrono>
#include <QMutex>
#include "errordetection.h"

using namespace std;

typedef chrono::time_point<chrono::high_resolution_clock> TimePoint;
typedef chrono::duration<double> Duration;

vector<Dictionary> dictionaries;
int currentDictionary;
QString autoDetectedDictionary = "";
int autoDetectedDictionaryID = -1;
const int autoDetect = -1;

int getCurrentDictionary() {
	return currentDictionary == autoDetect ? autoDetectedDictionaryID : currentDictionary;
}

Error::Error(ErrorType type, int startIndex, int endIndex, QString text) {
	this->type = type;
	this->startIndex = startIndex;
	this->endIndex = endIndex;
	this->text = text;
}

void Error::getSuggestions() {
	TimePoint start = chrono::high_resolution_clock::now();
	int dict = getCurrentDictionary();
	if(type == invalidWord && dictionaries.size() > dict) {
		this->suggestions = dictionaries[dict].words.closestMatches(this->text.toLower().toStdString(), 10);
		qDebug() << "Suggestions for " << this->text << ":" << this->suggestions;
	}

	if(this->suggestions.size() > 5) {
		this->suggestions.resize(5);
	}
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
}

Dictionary::Dictionary(string path) : words() {
	this->path = path;
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
	
	// aláhúzzuk a hibákat
	QString text = textEdit->toPlainText();
	textEdit->blockSignals(true);
	QTextCharFormat format;
	format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	format.setUnderlineColor(QColor(style::accentColor));
	for(Error &error : focusedFile->errors) {
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