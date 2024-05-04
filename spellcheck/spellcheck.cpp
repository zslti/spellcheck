#include <string>
#include <future>
#include <QtConcurrent/QtConcurrentRun>
#include "spellcheck.h"
#include "utils.h"

bool justSwithedFile = false;
bool acceptingPopupSelection = false;
QString popupSelectedText = "";
Settings settings;

Settings::Settings() {
    this->autoCorrect = this->areSuggestionsEnabled = true;
	errorTypes[ErrorType::invalidWord] = ErrorTypeSetting("Invalid words");
}

FileTab::FileTab(QPushButton* button, QPushButton* closeButton, string path, string text, int id, bool saved) : errors() {
    this->button = button;
    this->closeButton = closeButton;
    this->path = path;
    this->text = text;
    this->id = id;
    this->saved = saved;
    this->errorDetectionTime = -1;
    detectErrors(QString::fromStdString(text));
}

void FileTab::destroy() {
	if(button != nullptr) delete button;
    if(closeButton != nullptr) delete closeButton;
}

void spellcheck::destroyPopup() {
	if(this->popup == nullptr) return;
	delete this->popup;
	this->popup = nullptr;
}

Popup::Popup(spellcheck* parent, int x, int y, QString title, QString subtitle, vector<Popup::Button> buttons, int buttonHeight = 30, bool isOperableWithArrowKeys = false) {
    this->selectedIndex = 0;
    
    closeButton = new QPushButton(parent);
    QObject::connect(closeButton, &QPushButton::clicked, [=]() {parent->destroyPopup();});

	background = new QPushButton(parent);
	background->setStyleSheet(style::popupBackground);
	background->show();

	this->title = new QLabel(title, background);
    int titleWidth = this->title->sizeHint().width();
    int titleHeight = this->title->sizeHint().height();
    if(title == "") titleHeight = -14;
	this->title->setGeometry(10, 6, titleWidth + 10, titleHeight);
	this->title->setStyleSheet(style::popupTitle);
	this->title->show();

	this->subtitle = new QLabel(subtitle, background);
    int subtitleWidth = this->subtitle->sizeHint().width();
	int subtitleHeight = this->subtitle->sizeHint().height();
    if(subtitle == "") subtitleHeight = 0;
	this->subtitle->setGeometry(10, 26, subtitleWidth + 10, subtitleHeight);
	this->subtitle->setStyleSheet(style::popupSubtitle);
	this->subtitle->show();

    int maxWidth = max(titleWidth, subtitleWidth);

    bool selectionDone = false;
	for(int i = 0; i < buttons.size(); i++) {
    	QPushButton* button = new QPushButton(buttons[i].text, background);
        button->setStyleSheet(buttons[i].isHighlighted ? style::popupButtonHighlighted : style::popupButton);
        if(buttons[i].hasTopSeparator) button->setStyleSheet(button->styleSheet() + style::popupButtonWithTopSeparator);
        if(buttons[i].hasBottomSeparator) button->setStyleSheet(button->styleSheet() + style::popupButtonWithBottomSeparator);
    	button->show();
    	QObject::connect(button, &QPushButton::clicked, [=]() {
            buttons[i].onClick();
        });
    	this->buttons.push_back(button);
        maxWidth = max(maxWidth, button->sizeHint().width());
       
        if(isOperableWithArrowKeys && button->text() == popupSelectedText && (button->text() != "Add to dictionary" || acceptingPopupSelection)) {
        	button->setStyleSheet(style::popupButtonHighlighted);
			this->selectedIndex = i;
			selectionDone = true;
            acceptingPopupSelection = false;
        }
    }

    if(isOperableWithArrowKeys && !selectionDone && buttons.size() != 0 && this->buttons[0]->text() != "Add to dictionary") {
        this->buttons[0]->setStyleSheet(style::popupButtonHighlighted);
        popupSelectedText = this->buttons[0]->text();
    }

    maxWidth += popupWidthPadding;

    for(int i = 0; i < this->buttons.size(); i++) {
    	this->buttons[i]->setGeometry(0, titleHeight + subtitleHeight + 14 + i * buttonHeight, maxWidth + 20, buttonHeight);
    }
    int height = titleHeight + subtitleHeight + 14 + this->buttons.size() * buttonHeight;

    // maradjon a képernyőn belül
    x = parent->keepBetween(x, 0, parent->size().width() - maxWidth);
    y = parent->keepBetween(y, fileButtonHeight, parent->size().height() - height - bottomBarHeight);

    background->setGeometry(x, y, max(minPopupWidth, maxWidth), height);
}

Popup::~Popup() {
    qDebug() << "Popup destroyed";
    delete title;
    delete subtitle;
    for(QPushButton* button : buttons) {
    	delete button;
    }
    delete closeButton;
    delete background;
}

string getFileName(string str, bool includeExtension = true) {
    if(str[str.length() - 1] == '/' || str[str.length() - 1] == '\\') {
        str = str.substr(0, str.length() - 1);
    }

    size_t pos = str.find_last_of("/\\");
    if(pos != string::npos) {
    	str = str.substr(pos + 1);
    }

    if(str.size() > 15) {
		str = str.substr(0, 15) + "...";
	}

	if(!includeExtension) {
		pos = str.find_last_of(".");
		if(pos != string::npos) {
			str = str.substr(0, pos);
		}
	}
	return str;
}

QString spellcheck::getText() {
    return this->textEdit->toPlainText();
}

void spellcheck::detectLanguage() {
    QString text = textEdit->toPlainText();
    QString word = "";
    vector<int> validWords(dictionaries.size());
	for(int i = 0; i < text.size(); i++) {
		if(::isSeparator(text[i])) {
			if(word.length() == 0) continue;
            for(int dict = 0; dict < dictionaries.size(); dict++) {
            	validWords[dict] += dictionaries[dict].words.contains(word.toLower().toStdString());
                if(validWords[dict] > 10) {
                    autoDetectedDictionaryID = dict;
                    autoDetectedDictionary = " (" + QString::fromStdString(getFileName(dictionaries[dict].path, false)) + ")";
                    return;
                }
            }
			word = "";
		} else {
			word += text[i];
		}
	}

    int maxValue = 0, maxIndex = 0;
    for(int i = 0; i < validWords.size(); i++) {
    	if(validWords[i] > maxValue) {
            maxValue = validWords[i];
            maxIndex = i;
        }
    }

    if(maxValue > 3) {
        autoDetectedDictionaryID = maxIndex;
        autoDetectedDictionary = " (" + QString::fromStdString(getFileName(dictionaries[maxIndex].path, false)) + ")";
    } else {
        autoDetectedDictionaryID = -1;
    	autoDetectedDictionary = "";
    }
}

void spellcheck::updateBottomBarGeometry() {
    if(dictionaryButton == nullptr || autoCorrectButton == nullptr || errorsButton == nullptr) return;

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

    errorsButton->setText(QString::number(focusedFile->errors.size()) + " error" + (focusedFile->errors.size() != 1 ? "s" : ""));
    if(focusedFile->errorDetectionTime != -1) {
    	errorsButton->setText(errorsButton->text() + " (" + QString::number(focusedFile->errorDetectionTime) + " ms)");
    }
    int errorsButtonWidth = errorsButton->sizeHint().width();
    errorsButton->setGeometry(0, y - bottomBarHeight, errorsButtonWidth, bottomBarHeight);
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
        if(error.type == none) return;
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
    if(error.type == none) return;
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

    popup = new Popup(this, cursorRect.x(), cursorRect.y() + 50, "", "", buttons, 25, true);
    if(popup != nullptr && hasTextChanged) {
        popup->selectedIndex = 0;
        popupSelectedText = popup->buttons[0]->text();
        handleArrowUp(); // ez megcsinálja a highlightot
    }

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
        popup = new Popup(this, cursor.x(), cursor.y(), "Language", "", buttons);
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
            while(focusedFile->errors.size() != 0) {
                Error error = focusedFile->errors[0];
                QString suggestion = QString::fromStdString(error.getSuggestions()[0]);
                c.setPosition(error.startIndex);
                c.setPosition(error.endIndex, QTextCursor::KeepAnchor);
                c.insertText(suggestion);
                focusedFile->detectErrors(getText());
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
		popup = new Popup(this, 0, cursor.y(), "Errors", "Choose the type of errors to detect", buttons);
	});


    if(currentDictionary == -1) detectLanguage();
    updateBottomBarGeometry();
    focusedFile->detectErrors(getText());
    underlineErrors();
}

void spellcheck::onTextChanged() {
    if(justSwithedFile) {
    	justSwithedFile = false;
    	return;
    }

    QString text = textEdit->toPlainText();
    qDebug() << "Text changed:" << text;
    
    if(focusedFile != nullptr && focusedFile->saved) {
    	focusedFile->saved = false;
    	focusedFile->button->setText(focusedFile->button->text() + "*");
    }
}

void spellcheck::resizeEvent(QResizeEvent* event) {
    qDebug() << "Resized!";
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
        }
        addFileButton->setGeometry(addFileButton->x() + e - diff, 0, fileButtonHeight + 1, fileButtonHeight + 1);
    }
}

void spellcheck::createFileTabs(vector<FileTab> &tabs) {
    int maxFileID;
    if(fileTabs.size() == 0) {
        maxFileID = 0;
    } else {
        maxFileID = fileTabs[fileTabs.size() - 1].id + 1;
    }
    for(int i = 0; i < tabs.size(); i++) {
        int id = i + maxFileID;

        // file gombok létrehozása
		QPushButton* button = new QPushButton(QString::fromStdString(getFileName(tabs[i].path)), this);
		button->setGeometry(fileTabs.size() * (fileButtonWidth - 1) + fileTabScrollValue, 0, fileButtonWidth, fileButtonHeight + 1);
        button->setVisible(true);
        button->setStyleSheet(style::fileButton);
        connect(button, &QPushButton::clicked, this, [this, id]() {focusFile(id);});

        // file bezáró gombok létrehozása
        QPushButton* closeButton = new QPushButton("X", this);
        closeButton->setGeometry(fileTabs.size() * (fileButtonWidth - 1) + fileButtonWidth - 20 + fileTabScrollValue, 0, 20, fileButtonHeight + 1);
        closeButton->setStyleSheet(style::fileCloseButton);
        closeButton->setVisible(true);
        connect(closeButton, &QPushButton::clicked, this, [this, id]() {closeFile(id);});

        // file tartalmának betöltése
        ifstream file(tabs[i].path);
        stringstream buffer;
        buffer << file.rdbuf();
        file.close();

	    fileTabs.push_back(FileTab(button, closeButton, tabs[i].path, buffer.str(), id, tabs[i].saved));
	}
    if(focusedFile == nullptr) focusFile(fileTabs[0].id);

    // az új file gombot a fileok végéhez teszük
    addFileButton->setGeometry(fileTabs.size() * (fileButtonWidth - 1) + fileTabScrollValue, 0, fileButtonHeight + 1, fileButtonHeight + 1);

    underlineErrors();
}

vector<string> split(string str, char delimiter) {
	vector<string> res;
	stringstream ss(str);
	string s;
  
    while(getline(ss, s, delimiter)) {
        res.push_back(s);
	}
  
	return res;
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
	file << "0 1 1";
	file.close();
}

void spellcheck::restoreLastSession() {
    // beállítások betöltése
    ifstream settingsFile("data/settings.txt");
    if(!settingsFile.good()) restoreDefaultSettings();
    else {
        settingsFile >> settings.autoCorrect >> settings.areSuggestionsEnabled >> settings.errorTypes[invalidWord].enabled;
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
        	    qDebug() << "could not open file " << path;
        	    return;
            }
            dictionaries[dictionaries.size() - 1].words.load(file);
            file.close();
            qDebug() << "test " << dictionaries[dictionaries.size() - 1].words.contains("banana");
        }
        qDebug() << "Dictionaries loaded";
    }

    // fileok betöltése
	vector<string> files = getFilesFromLastSession();
    qDebug() << "Files from last session: " << files;
    if(files.empty()) files.push_back(getNewUntitledFile());
	vector<FileTab> tabs;
    for(int i = 0; i < files.size(); i++) {
		tabs.push_back({nullptr, nullptr, files[i], "", i, true});
    }
	createFileTabs(tabs);
}

void spellcheck::focusFile(int fileID) {
    justSwithedFile = true;
    // lementjük a jelenlegi fókuszban lévő file tartalmát
    if(focusedFile != nullptr) {
		focusedFile->text = textEdit->toPlainText().toStdString();
	}

    for(FileTab& tab : fileTabs) {
        if(tab.id == fileID) {
			tab.button->setStyleSheet(style::fileButtonFocused);
            textEdit->setText(QString::fromStdString(tab.text));
            focusedFile = &tab;
        } else {
			tab.button->setStyleSheet(style::fileButton);
		}
	}

    detectLanguage();
    focusedFile->detectErrors(getText());
    underlineErrors();
    updateBottomBarGeometry();
}

void spellcheck::saveFile() {
	if(focusedFile == nullptr) return;
	ofstream file(focusedFile->path);
	file << textEdit->toPlainText().toStdString();
	file.close();
	focusedFile->saved = true;
	focusedFile->button->setText(QString::fromStdString(getFileName(focusedFile->path)));
}

void spellcheck::closeFile(int fileID) {
    qDebug() << "Closing file with id: " << fileID;

	// kitöröljük a jelenlegi file gombot
    int i = 0;
    int deletedI = -1;
    for(i; i < fileTabs.size(); i++) {
        if(fileTabs[i].id == fileID) {
            // ha a törölt file volt a fókuszban, akkor az elsőt állítjuk fókuszba
            bool wasFocused = (focusedFile == &fileTabs[i] && fileTabs.size() > 1);
            qDebug() << "was focused: " << wasFocused;

            deletedI = i;

            fileTabs[i].destroy();
            fileTabs.erase(fileTabs.begin() + i);

            if(wasFocused) {
                focusedFile = nullptr;
                focusFile(fileTabs[0].id);
            }
			break;
		}
	}

    // ha a fókuszált file előtti filet zártuk be a focusedFile-t egyel vissza tesszük
    for(int i = 0; i < fileTabs.size(); i++) {
        if(fileTabs[i].id == focusedFile->id) {
            if(deletedI < i) {
                focusedFile = &fileTabs[i - 1];
            }
            break;
        }
    }

    // balra toljuk a többi file gombot
    for(i; i < fileTabs.size(); i++) {
        fileTabs[i].button->setGeometry(i * (fileButtonWidth - 1) + fileTabScrollValue, 0, fileButtonWidth, fileButtonHeight + 1);
        fileTabs[i].closeButton->setGeometry(i * (fileButtonWidth - 1) + fileButtonWidth - 20 + fileTabScrollValue, 0, 20, fileButtonHeight + 1);
    }

    // toljuk balra az új file gombot
    addFileButton->setGeometry(fileTabs.size() * (fileButtonWidth - 1) + fileTabScrollValue, 0, fileButtonHeight + 1, fileButtonHeight + 1);
}   

void spellcheck::addFile() {
	QString path = QFileDialog::getOpenFileName(this, "Open file");
	if(path.isEmpty()) return;

    FileTab tab(path.toStdString());
    vector<FileTab> tabs = {tab};
    createFileTabs(tabs);
    focusFile(fileTabs[fileTabs.size() - 1].id);
}

void spellcheck::saveCurrentSession() {
    // beállítások mentése
    ofstream settingsFile("data/settings.txt");
    settingsFile << settings.autoCorrect << " " << settings.areSuggestionsEnabled << " " << settings.errorTypes[invalidWord].enabled;

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

bool spellcheck::fileExists(string path) {
	ifstream file(path);
    bool good = file.good();
    file.close();
    return good;
}

string spellcheck::getNewUntitledFile() {
    static int counter = 0;
    string fileName = "untitled" + (counter == 0 ? "" : "-" + to_string(counter)) + ".txt";
    counter++;
    if(fileExists(fileName)) return getNewUntitledFile();
	return fileName;
 }

void spellcheck::addUntitledFile() {
    qDebug() << "Adding new file";
	string fileName = getNewUntitledFile();
    FileTab tab(fileName);
	vector<FileTab> tabs = {tab};
	createFileTabs(tabs);
    focusedFile = nullptr;
	focusFile(fileTabs[fileTabs.size() - 1].id);
}

int spellcheck::keepBetween(int value, int min, int max) {
	if(value < min) return min;
	if(value > max) return max;
	return value;
}

void spellcheck::handleArrow(bool direction) {
    if(popup == nullptr) return;
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