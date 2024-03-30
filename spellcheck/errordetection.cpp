#include <iostream>
#include "errordetection.h"

using namespace std;

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
	this->errors.clear();
	text += " ";

	// helytelen szó ellenőrzés
	QString word = "";
	for(int i = 0; i < text.size(); i++) {
		if(isSeparator(text[i])) {
			if(word.length() == 0) continue;
			bool isValid = dictionaries[currentDictionary].words.contains(word.toStdString());
			if(!isValid) this->errors.push_back(Error(invalidWord, i - word.length(), i, word));
			word = "";
		} else {
			word += text[i];
		}
	}
}

void spellcheck::underlineErrors() {
	QTextCursor cursor = textEdit->textCursor();

    // töroljuk az előző hibákat
    cursor.select(QTextCursor::Document);
    textEdit->blockSignals(true);
    cursor.setCharFormat(QTextCharFormat());
    textEdit->blockSignals(false);
    cursor.clearSelection();

	QString text = textEdit->toPlainText();
	for(Error &error : focusedFile->errors) {
		cursor.setPosition(error.startIndex);
		cursor.setPosition(error.endIndex, QTextCursor::KeepAnchor);
		QTextCharFormat format;
		format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
		format.setUnderlineColor(Qt::red);
		textEdit->blockSignals(true);
		cursor.setCharFormat(format);
		textEdit->blockSignals(false);
	}
}