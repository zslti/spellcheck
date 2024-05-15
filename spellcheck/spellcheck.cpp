#include "spellcheck.h"

bool justSwitchedFile = false;
bool acceptingPopupSelection = false;
QString popupSelectedText = "";
Settings settings;

Settings::Settings() {
    this->autoCorrect = this->areSuggestionsEnabled = true;
	errorTypes[ErrorType::invalidWord] = ErrorTypeSetting("Invalid words");
    errorTypes[ErrorType::whitespace] = ErrorTypeSetting("Whitespace");
    errorTypes[ErrorType::capitalization] = ErrorTypeSetting("Capitalization");
    errorTypes[ErrorType::repeatedWords] = ErrorTypeSetting("Repeated words");
}

QString spellcheck::getText() {
    return this->textEdit->toPlainText();
}

void spellcheck::updateBottomBarGeometry() {
    if(dictionaryButton == nullptr || autoCorrectButton == nullptr || errorsButton == nullptr || suggestionsButton == nullptr) return;

    if(currentDictionary != -1) {
        dictionaryButton->setText(QString::fromStdString(getFileName(dictionaries[currentDictionary].path, false)));
    } else {
        dictionaryButton->setText("Auto detect" + autoDetectedDictionary);
    }
    dictionaryButton->setGeometry(this->size().width() - dictionaryButton->sizeHint().width(), dictionaryButton->geometry().top(), dictionaryButton->sizeHint().width(), dictionaryButton->size().height());
    

    int x = this->size().width();
    int y = this->size().height();

    bottomBar->setGeometry(0, y - bottomBarHeight - 1, x, bottomBarHeight + 1);

    int dictionaryButtonWidth = dictionaryButton->sizeHint().width();
    dictionaryButton->setGeometry(x - dictionaryButtonWidth, y - bottomBarHeight, dictionaryButtonWidth, bottomBarHeight);

    autoCorrectButton->setText(settings.autoCorrect ? "Autocorrect on" : "Autocorrect off");
    int autoCorrectButtonWidth = autoCorrectButton->sizeHint().width();
    autoCorrectButton->setGeometry(x - dictionaryButtonWidth - autoCorrectButtonWidth, y - bottomBarHeight, autoCorrectButtonWidth, bottomBarHeight);

    errorsButton->setText(QString::number(focusedFile->errorCount) + " error" + (focusedFile->errorCount != 1 ? "s" : ""));
    if(focusedFile->errorDetectionTime != -1) {
    	errorsButton->setText(errorsButton->text() + " (" + QString::number(focusedFile->errorDetectionTime) + "ms)");
    }
    int errorsButtonWidth = errorsButton->sizeHint().width();
    errorsButton->setGeometry(0, y - bottomBarHeight, errorsButtonWidth, bottomBarHeight);
    
    if(popup != nullptr && popup->buttons.size() > 1) {
        suggestionsButton->show();
        suggestionsButton->setText(QString("Suggestions found in ") + QString::number(focusedFile->suggestionsTime) + QString("ms"));
	    int suggestionsButtonWidth = suggestionsButton->sizeHint().width();
	    suggestionsButton->setGeometry(errorsButtonWidth, y - bottomBarHeight, suggestionsButtonWidth, bottomBarHeight);
        
        bool isWordSuggestion = false;
        for(QPushButton* button : popup->buttons) {
            if(button->text() == "Add to dictionary") {
				isWordSuggestion = true;
				break;
			}
		}
        if(!isWordSuggestion) suggestionsButton->hide();
    } else {
        suggestionsButton->hide();
    }

    if(suggestionsButton->geometry().right() > autoCorrectButton->geometry().left()) suggestionsButton->hide();
}

void spellcheck::onCursorChanged() {
    static string oldText = "";
    if(focusedFile == nullptr) return;
    destroyPopup();
    bool hasTextChanged = oldText != textEdit->toPlainText().toStdString();

    // ha kijelöljük a szöveget, ne csináljon semmit, arra sokszor hívódik meg
    if(textEdit->textCursor().selectedText().length() > 0) return;
    
    QTextCursor cursor = textEdit->textCursor();
    QChar currentChar = textEdit->toPlainText().at(cursor.position() - 1);
    if(currentChar == '\x9' || (currentChar == ' ' && settings.autoCorrect)) { // tab
        Error error = getErrorAt(textEdit->textCursor().position() - 1, getText(), focusedFile->errors);
        if(error.type == none || error.text == " ") return;
    	if(textEdit->toPlainText().count('\x9') > QString::fromStdString(oldText).count('\x9') || (textEdit->toPlainText().count(' ') > QString::fromStdString(oldText).count(' ') && settings.autoCorrect)) {
            QTextCursor c = textEdit->textCursor();
            c.setPosition(cursor.position() - 1);
            c.setPosition(cursor.position(), QTextCursor::KeepAnchor);
            c.removeSelectedText();
            acceptPopupSelection();
            if(currentChar == ' ') c.insertText(" ");
        }
    }

    if(hasTextChanged) {
        if(currentDictionary == autoDetect) detectLanguage();
        focusedFile->detectErrors(textEdit->toPlainText());
        focusedFile->errors.size() < 100 ? underlineErrors() : underlineErrorsLater();
        updateBottomBarGeometry();
    }
    oldText = textEdit->toPlainText().toStdString();

    Error error = getErrorAt(textEdit->textCursor().position(), getText(), focusedFile->errors);
    if(error.type == none) {
        updateBottomBarGeometry();
        return;
    }
    error.getSuggestions();

    QRect cursorRect = textEdit->cursorRect(cursor);
    vector<Popup::Button> buttons;
    for(string &suggestion : error.suggestions) {
        buttons.push_back({QString::fromStdString(suggestion), [=]() {
            QTextCursor c = textEdit->textCursor();
            c.setPosition(error.startIndex);
            c.setPosition(error.endIndex, QTextCursor::KeepAnchor);
            c.insertText(QString::fromStdString(suggestion));
            focusedFile->detectErrors(getText());

            underlineErrors();
            updateBottomBarGeometry();
            destroyPopup();

            highlightCorrectedWord(error.startIndex, error.startIndex + suggestion.length());
        }}); 
    }

    if(error.type == invalidWord) {
        int dict = getCurrentDictionary();
    	buttons.push_back({"Add to dictionary", [=]() {
            dictionaries[dict].words.insert(error.text.toStdString());
            dictionaries[dict].changed = true;
            focusedFile->detectErrors(getText());
            underlineErrors();
            updateBottomBarGeometry();
            destroyPopup();
        }, false, true}); 
    }

    if(buttons.size() == 0) return;

    popup = new Popup(this, cursorRect.x(), cursorRect.y() + 50, "", "", buttons, 25, true);
    if(popup != nullptr && hasTextChanged) {
        popup->selectedIndex = 0;
        popupSelectedText = popup->buttons[0]->text();
        handleArrowUp(); // ez megcsinálja a highlightot
    }

    updateBottomBarGeometry();
    if(!settings.areSuggestionsEnabled) popup->background->hide();
}

spellcheck::spellcheck(QWidget *parent) : QMainWindow(parent) {
    setMinimumSize(400, 300);
    this->setMouseTracking(true);
       
    bottomBar = new QPushButton(this);
    bottomBar->setStyleSheet(style::bottomBar);

    background = new QPushButton(this);
    background->setGeometry(0, 0, 400, 300 - bottomBarHeight);
    background->setStyleSheet(style::background);
    connect(background, &QPushButton::clicked, this, &spellcheck::addUntitledFile);

    textEdit = new QTextEdit(this);
    connect(textEdit, &QTextEdit::textChanged, this, &spellcheck::onTextChanged);
    connect(textEdit, &QTextEdit::cursorPositionChanged, this, &spellcheck::onCursorChanged);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this), &QShortcut::activated, this, &spellcheck::saveFile);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), this), &QShortcut::activated, this, &spellcheck::handleArrowDown);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), this), &QShortcut::activated, this, &spellcheck::handleArrowUp);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Period), this), &QShortcut::activated, this, &spellcheck::acceptPopupSelection);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab), this), &QShortcut::activated, this, &spellcheck::acceptPopupSelection);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Space), this), &QShortcut::activated, this, &spellcheck::insertSpaceWithoutAutoCorrect);
    
    QFont font = textEdit->font();
    font.setPointSize(10);
    textEdit->setFont(font);

    addFileButton = new QPushButton("+", this);
    addFileButton->setGeometry(0, 0, fileButtonHeight + 1, fileButtonHeight + 1);
    addFileButton->setStyleSheet(style::fileAddButton);
    connect(addFileButton, &QPushButton::clicked, this, &spellcheck::addFile);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Highlight, QColor(style::highlightColor));
    this->setPalette(palette);

    restoreLastSession();

    dictionaryButton = new QPushButton((currentDictionary != -1 ? QString::fromStdString(getFileName(dictionaries[currentDictionary].path, false)) : "Language"), this);
    dictionaryButton->setStyleSheet(style::bottomBarButton);
    connect(dictionaryButton, &QPushButton::clicked, this, [this]() {
        destroyPopup();
        // az elejére visszük a kurzort, mert onCursorChange-en lesz destroyolva a popup, 
        // és ha oda kattintanánk ahol volt eddig, akkor nem fut le az event
        textEdit->moveCursor(QTextCursor::Start);
        destroyPopup();

        vector<Popup::Button> buttons;
        buttons.push_back({"Auto detect", [=]() {
            currentDictionary = autoDetect;
            detectLanguage();
			focusedFile->detectErrors(getText());
			underlineErrors();
			updateBottomBarGeometry();
			destroyPopup();
        }, currentDictionary == autoDetect, false, true});

        // szótárak gombjai
        for(int i = 0; i < dictionaries.size(); i++) {
            QString dictionaryName = QString::fromStdString(getFileName(dictionaries[i].path, false));
            buttons.push_back({dictionaryName, [=]() {
				currentDictionary = i;
                focusedFile->detectErrors(getText());
				underlineErrors();
                updateBottomBarGeometry();
				destroyPopup();
			}, i == currentDictionary});
        }

        buttons.push_back({"Add new dictionary", [=]() {
            QString path = QFileDialog::getOpenFileName(this, "Open dictionary");
            if(path.isEmpty()) return;
            ifstream file(path.toStdString());
            dictionaries.push_back(Dictionary(file, QString::fromStdString(getFileName(path.toStdString(), false))));
            file.close();
            currentDictionary = dictionaries.size() - 1;
            focusedFile->detectErrors(getText());
            underlineErrors();
            updateBottomBarGeometry();
            destroyPopup();
        }, false, true});
        QPoint cursor = QWidget::mapFromGlobal(QCursor::pos());
        popup = new Popup(this, cursor.x(), cursor.y(), "Language", "", buttons, 30, false);
    });

    autoCorrectButton = new QPushButton("Autocorrect", this);
    autoCorrectButton->setStyleSheet(style::bottomBarButton);
    connect(autoCorrectButton, &QPushButton::clicked, this, [this]() {
        settings.autoCorrect = !settings.autoCorrect;
        autoCorrectButton->setText(settings.autoCorrect ? "Autocorrect on" : "Autocorrect off");
        destroyPopup();
    });

    errorsButton = new QPushButton("0 errors", this);
    errorsButton->setStyleSheet(style::bottomBarButton);
    connect(errorsButton, &QPushButton::clicked, this, [=]() {
		destroyPopup();

        // az elejére visszük a kurzort, mert onCursorChange-en lesz destroyolva a popup, 
        // és ha oda kattintanánk ahol volt eddig, akkor nem fut le az event
        textEdit->moveCursor(QTextCursor::Start);
        destroyPopup();
		vector<Popup::Button> buttons;

        for(pair<const int, ErrorTypeSetting> &type : settings.errorTypes) {
            buttons.push_back({type.second.name, [=]() {
				settings.errorTypes[type.first].enabled = !settings.errorTypes[type.first].enabled;
				focusedFile->detectErrors(getText());
				underlineErrors();
				updateBottomBarGeometry();
				destroyPopup();
                errorsButton->click();
			}, type.second.enabled});
		}
        
        buttons.push_back({"Fix all errors", [=]() {
            QTextCursor c = textEdit->textCursor();
            while(focusedFile->errorCount != 0) {
                int i = 0;
                Error error = focusedFile->errors[i];
                while(error.type == none) error = focusedFile->errors[++i];
                QString suggestion = QString::fromStdString(error.getSuggestions()[0]);
                c.setPosition(error.startIndex);
                c.setPosition(error.endIndex, QTextCursor::KeepAnchor);
                c.insertText(suggestion);
                focusedFile->detectErrors(getText());

                highlightCorrectedWord(error.startIndex, error.startIndex + suggestion.length());
            }

            focusedFile->detectErrors(getText());
            underlineErrors();
            updateBottomBarGeometry();
            destroyPopup();
        }, false, true});

        buttons.push_back({(settings.areSuggestionsEnabled ? "Disable" : "Enable") + QString(" suggestions"), [=]() {
            settings.areSuggestionsEnabled = !settings.areSuggestionsEnabled;
            updateBottomBarGeometry();
            destroyPopup();
        }});

		QPoint cursor = QWidget::mapFromGlobal(QCursor::pos());
		popup = new Popup(this, 0, cursor.y(), "Errors", "Choose the type of errors to detect", buttons, 30, false);
	});

    suggestionsButton = new QPushButton("", this);
    suggestionsButton->setStyleSheet(style::bottomBarButton);

    if(currentDictionary == -1) detectLanguage();
    updateBottomBarGeometry();
    focusedFile->detectErrors(getText());
    underlineErrors();
}

void spellcheck::onTextChanged() {
    if(justSwitchedFile) {
    	justSwitchedFile = false;
    	return;
    }

    if(focusedFile != nullptr && focusedFile->saved) {
    	focusedFile->saved = false;
    	focusedFile->button->setText(focusedFile->button->text() + "*");
    }
}

void spellcheck::resizeEvent(QResizeEvent* event) {
    int x = event->size().width();
    int y = event->size().height();
    textEdit->setGeometry(0, fileButtonHeight, x, y - fileButtonHeight - bottomBarHeight);
    background->setGeometry(0, 0, x, y - bottomBarHeight);
    updateBottomBarGeometry();
}

void spellcheck::wheelEvent(QWheelEvent* event) {   
    QPoint cursor = QWidget::mapFromGlobal(QCursor::pos());

    if(cursor.y() < fileButtonHeight) { // ha a file bar-on van a kurzor
        int e = keepBetween(event->angleDelta().x() + event->angleDelta().y(), -50, 50) / 2; // mekkorát scrolloltunk

        fileTabScrollValue += e;
        int scrollValue = fileTabScrollValue;

        // ne lehessen a túl oldalra menni
        fileTabScrollValue = keepBetween(fileTabScrollValue, min(0, this->size().width() - fileButtonWidth * ((int)fileTabs.size() + 1)), 0);

        int diff = scrollValue - fileTabScrollValue;

        // minden gombot eltolunk amennyivel kell
        for(FileTab &tab : fileTabs) {
        	tab.button->setGeometry(tab.button->x() + e - diff, 0, fileButtonWidth, fileButtonHeight + 1);
            tab.closeButton->setGeometry(tab.closeButton->x() + e - diff, 0, 20, fileButtonHeight + 1);
        }
        addFileButton->setGeometry(addFileButton->x() + e - diff, 0, fileButtonHeight + 1, fileButtonHeight + 1);
    }
}

vector<string> spellcheck::getFilesFromLastSession() {
	vector<string> files;
	ifstream file("data/lastsession.txt");
    if(!file) return files;
    string lastSession;
    file >> lastSession;
    vector<string> fileNames = split(lastSession, '|');
    for(string &fileName : fileNames) {
        ifstream f(fileName);
        if(f.good()) files.push_back(fileName);
        f.close();
	}
	return files;
}

void spellcheck::restoreDefaultSettings() {
	ofstream file("data/settings.txt");
	file << "0 1 1 1 1 1";
	file.close();
}

void spellcheck::restoreLastSession() {
    // beállítások betöltése
    ifstream settingsFile("data/settings.txt");
    if(!settingsFile.good()) restoreDefaultSettings();
    else {
        settingsFile >> settings.autoCorrect >> settings.areSuggestionsEnabled;
        for(pair<const int, ErrorTypeSetting> &type : settings.errorTypes) {
			settingsFile >> type.second.enabled;
		}
    }

    // szótárak betöltése
    ifstream file("data/dictionaries.txt");
    currentDictionary = -1;
    string str;
    if(file >> currentDictionary >> str) {
        file.close();
        vector<string> paths = split(str, '|');
        for(string &path : paths) {
            dictionaries.push_back(Dictionary());
            dictionaries[dictionaries.size() - 1].path = path;
            ifstream file("data/dictionaries/" + path);
            if(!file.good()) {
        	    continue;
            }
            dictionaries[dictionaries.size() - 1].words.load(file);
            file.close();
        }
    }

    // fileok betöltése
	vector<string> files = getFilesFromLastSession();
    if(files.empty()) files.push_back(getNewUntitledFile());
	vector<FileTab> tabs;
    for(int i = 0; i < files.size(); i++) {
		tabs.push_back({nullptr, nullptr, files[i], "", i, true});
    }
	createFileTabs(tabs);
}

void spellcheck::saveCurrentSession() {
    // beállítások mentése
    ofstream settingsFile("data/settings.txt");
    settingsFile << settings.autoCorrect << " " << settings.areSuggestionsEnabled << " ";
    for(pair<const int, ErrorTypeSetting> &type : settings.errorTypes) {
		settingsFile << type.second.enabled << " ";
	}

    // nyitott fileok mentése
    ofstream file("data/lastsession.txt");
	string lastSession = "";
	for(FileTab &tab : fileTabs) {
    	lastSession += tab.path + "|";
    }
	file << lastSession;
	file.close();

    // szótárak mentése
    ofstream file2("data/dictionaries.txt");
    file2 << currentDictionary << "\n";
    for(Dictionary &dictionary : dictionaries) {
        if(dictionary.changed) {
            ofstream file("data/dictionaries/" + dictionary.path);
            dictionary.words.save(file);
            file.close();
        }
		file2 << dictionary.path << "|";
	}
}

void spellcheck::closeEvent(QCloseEvent *event) {
	saveCurrentSession();
	event->accept();
}

void spellcheck::handleArrow(bool direction) {
    if(popup == nullptr || !popup->isOperableWithArrowKeys) return;
    popup->selectedIndex = keepBetween(direction ? --popup->selectedIndex : ++popup->selectedIndex, 0, popup->buttons.size() - 1);
    for(QPushButton* button : popup->buttons) {
        button->setStyleSheet(button == popup->buttons[popup->selectedIndex] ? style::popupButtonHighlighted : style::popupButton);
        if(button->text() == "Add to dictionary") button->setStyleSheet(button->styleSheet() + style::popupButtonWithTopSeparator);
    }
    QString text = popup->buttons[popup->selectedIndex]->text();
    popupSelectedText = text;
    acceptingPopupSelection = (text == "Add to dictionary");
}

void spellcheck::handleArrowDown() {
    handleArrow(false);
}

void spellcheck::handleArrowUp() {
    handleArrow(true);
}

void spellcheck::acceptPopupSelection() {
	if(popup == nullptr) return;
    if(popup->buttons.size() == 0) {destroyPopup(); return;};
    if(popup->buttons.size() == 1 && popup->buttons[0]->text() == "Add to dictionary") {destroyPopup(); return;};
    acceptingPopupSelection = true;
	popup->buttons[popup->selectedIndex]->click();
}

void spellcheck::insertSpaceWithoutAutoCorrect() {
    bool oldAutoCorrect = settings.autoCorrect;
    settings.autoCorrect = false;
	QTextCursor cursor = textEdit->textCursor();
	cursor.insertText(" ");
    settings.autoCorrect = oldAutoCorrect;
    updateBottomBarGeometry();
}

void spellcheck::highlightCorrectedWord(int start, int end, double progress) {
    QTextCharFormat format;
    QTextCursor c = textEdit->textCursor();

    QColor accentColor = QColor(style::errorCorrectionColor);
    QColor defaultColor = QColor("#ffffff");
    QColor color = interpolateColors(accentColor, defaultColor, progress);

    format.setForeground(QBrush(color));
    c.setPosition(start);
    c.setPosition(end, QTextCursor::KeepAnchor);
    c.setCharFormat(format);

    progress = keepBetween(progress + 0.02, 0.0, 1.0);
    if(progress < 1) QTimer::singleShot(10, [=]() {
        highlightCorrectedWord(start, end, progress);
    });
}