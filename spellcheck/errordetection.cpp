#include <iostream>
#include <chrono>
#include <QMutex>
#include <QRegularExpression>
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

vector<string>& Error::getSuggestions(int dict) {
	TimePoint start = chrono::high_resolution_clock::now();
	this->suggestions.clear();
	if(dict == -1) dict = getCurrentDictionary();
	if(type == invalidWord && dictionaries.size() > dict) {
		if(this->text.size() < 2) return this->suggestions;

		TimePoint start = chrono::high_resolution_clock::now();
		dictionaries[dict].words.closestMatches(this->text.toLower().toStdString(), 5, 500, this->suggestions);
		Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;

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

		// ha valahova beszúrható egy betű és az valid betesszük
		for(int i = 0; i <= this->text.size(); i++) {
			for(const QChar &c : validChars) {
				QString word = this->text;
				word.insert(i, c);
				if(dictionaries[dict].words.contains(word.toLower().toStdString())) this->suggestions.push_back(word.toStdString());
			}
		}

		// ha a szó kettéválasztható két valid szóvá szedjük szét
		for(int i = 0; i < this->text.size() - 1; i++) {
			QString word1 = this->text.left(i + 1);
			QString word2 = this->text.right(this->text.size() - i - 1);
			if(dictionaries[dict].words.contains(word1.toLower().toStdString()) && dictionaries[dict].words.contains(word2.toLower().toStdString())) {
				this->suggestions.push_back(word1.toStdString() + " " + word2.toStdString());
				this->suggestions.push_back(word1.toStdString() + "-" + word2.toStdString());
			}
		}

		removeDuplicates(this->suggestions);
		sortByRelevance(this->suggestions, this->text.toStdString());

		for(string &suggestion : this->suggestions) {
			suggestion = maintainCase(suggestion, this->text.toStdString());			
		}
	} else {
		suggestions.push_back(this->text.toStdString());
	}

	if(this->suggestions.size() > 5) this->suggestions.resize(5);
	
	Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
	spellcheck::focusedFile->suggestionsTime = elapsedSeconds.count() * 1000;
	return this->suggestions;
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
	if(!file.good()) return;
	words.load(file);
	file.close();
}

Dictionary::Dictionary(ifstream &file, QString fileName) : Dictionary()  {
    if(!file.good()) return;
    this->words = RedBlackTree<string>(file);
	this->changed = false;
	string path = "data/dictionaries/" + fileName.toStdString() + ".dict";
	ofstream out(path);
	this->words.save(out);
	this->path = (fileName + ".dict").toStdString();
}

Dictionary::Dictionary(const Dictionary &d) {
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
	QString separators = " \n\t.,?!()[]{}:;-+~=_/\"'`";
	return separators.contains(c);
}

void FileTab::detectErrors(QString text) {
	TimePoint start = chrono::high_resolution_clock::now();
	this->errors.clear();
	text += "\n";

	int dict = getCurrentDictionary();

	// helytelen szó ellenőrzés
	if(settings.errorTypes[invalidWord].enabled && dictionaries.size() > dict) {
		QString word = "";
		for(int i = 0; i < text.size(); i++) {
			if(isSeparator(text[i])) {
				if(word.length() == 0) continue;
				bool isValid = dictionaries[dict].words.contains(word.toLower().toStdString());
				if(word.length() < 2) isValid = true; // egy darab betu legyen mindig valid
				if(!isValid) this->errors.push_back(Error(invalidWord, i - word.length(), i, word));
				word = "";
			} else {
				word += text[i];
			}
		}
	}

	if(settings.errorTypes[whitespace].enabled) {
		// punktuáció előtti szóközök
		QString punctuation = ".,?!:;";
		for(int i = 0; i < text.size() - 1; i++) {
			if(text[i] == ' ' && punctuation.contains(text[i + 1])) {
				int start = i;
				while(start > 0 && text[start - 1] == ' ') start--;
				this->errors.push_back(Error(whitespace, start, i + 2, QString(text[i + 1])));
			}
		}

		// többszörös szóközök
		for(int i = 0; i < text.size(); ++i) {
			if(text[i] == ' ') {
				int start = i, end = i;
				while(end < text.size() && text[end] == ' ') end++;

				if(--end > start) this->errors.push_back(Error(whitespace, start, end + 1, " "));
				i = end;
			}
		}
	}

	if(settings.errorTypes[capitalization].enabled) {
		// kisbetűve kezdődő szavak amik naggyal kéne kezdődjenek
		QString punctuation = ".?!";
		QString word = "";
		for(int i = 0; i < text.size(); i++) {
			if(isSeparator(text[i])) {
			 	if(word.length() == 0) continue;
				int wordStart = i - word.length();

				bool shouldStartWithCapital = false;
				if(wordStart == 0) shouldStartWithCapital = true;
				else if(wordStart >= 2 && text[wordStart - 1] == ' ' && punctuation.contains(text[wordStart - 2])) shouldStartWithCapital = true;

				if(word[0].isLower() && shouldStartWithCapital) {
					word[0] = word[0].toUpper();
					this->errors.push_back(Error(capitalization, wordStart, i, word));
				}
			 	word = "";
			} else {
			 	word += text[i];
			}
		}
	}

	if(settings.errorTypes[repeatedWords].enabled) {
		// egymás után kétszer ugyanaz a szó
		QString word = "", prevWord = "";
		int prevWordStart = 0;
		for(int i = 0; i < text.size(); i++) {
			if(isSeparator(text[i])) {
				if(word.length() == 0) continue;
				if(word == prevWord) this->errors.push_back(Error(repeatedWords, prevWordStart, i, word));
				prevWord = word;
				prevWordStart = i - word.length();
				word = "";
			} else {
				word += text[i];
			}
		}
	}
	
	// vegyük ki a hibák közül a hamis pozitívakat
	this->errorCount = this->errors.size();
	for(Error &error : this->errors) {
		QRegularExpression numberOrPhoneNumber("^\\+*[0-9]+$");

		vector<QRegularExpression> falsePositives = {numberOrPhoneNumber};
		for(QRegularExpression &regex : falsePositives) if(regex.match(error.text).hasMatch()) {errorCount--; error.type = none;};
	}

	Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
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
	for(Error &error : focusedFile->errors) {
		if(error.type == none) continue;
		cursor.setPosition(error.startIndex);
		cursor.setPosition(error.endIndex, QTextCursor::KeepAnchor);
		cursor.setCharFormat(format);
	}
	textEdit->blockSignals(false);

	Duration elapsedSeconds = chrono::high_resolution_clock::now() - start;
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