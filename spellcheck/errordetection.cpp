#include <iostream>
#include <chrono>
#include <QMutex>
#include "errordetection.h"

using namespace std;

typedef chrono::time_point<chrono::high_resolution_clock> TimePoint;
typedef chrono::duration<double> Duration;

vector<Dictionary> dictionaries;
int currentDictionary;

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

	// helytelen szó ellenőrzés
	if(currentDictionary != -1 && dictionaries.size() > currentDictionary) {
		QString word = "";
		for(int i = 0; i < text.size(); i++) {
			if(isSeparator(text[i])) {
				if(word.length() == 0) continue;
				bool isValid = dictionaries[currentDictionary].words.contains(word.toLower().toStdString());
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