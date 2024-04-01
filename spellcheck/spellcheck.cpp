#include <string>
#include <future>
#include <QtConcurrent/QtConcurrentRun>
#include "spellcheck.h"

bool justSwithedFile = false;

FileTab::FileTab(QPushButton* button, QPushButton* closeButton, string path, string text, int id, bool saved) : errors() {
    this->button = button;
    this->closeButton = closeButton;
    this->path = path;
    this->text = text;
    this->id = id;
    this->saved = saved;
    detectErrors(QString::fromStdString(text));
}

void FileTab::destroy() {
	if(button != nullptr) delete button;
    if(closeButton != nullptr) delete closeButton;
}

Popup::Popup(spellcheck* parent, int x, int y, QString title, QString subtitle, vector<pair<QString, function<void(int, int, QString)>>> buttons) {
    closeButton = new QPushButton(parent);
    closeButton->setGeometry(0, 0, parent->size().width(), parent->size().height());
    closeButton->setStyleSheet("background: transparent");
    closeButton->show();
    QObject::connect(closeButton, &QPushButton::clicked, [=]() {
        delete this;
    });

	background = new QPushButton(parent);
	//background->setGeometry(x, y, 300, 200);
	background->setStyleSheet(style::popupBackground);
	background->show();

	this->title = new QLabel(title, background);
    int titleWidth = this->title->sizeHint().width();
    int titleHeight = this->title->sizeHint().height();
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

	for(int i = 0; i < buttons.size(); i++) {
    	QPushButton* button = new QPushButton(buttons[i].first, background);
    	//button->setGeometry(10, 80 + i * 30, 280, 30);
    	button->setStyleSheet(style::popupButton);
    	button->show();
    	QObject::connect(button, &QPushButton::clicked, [=]() {
            buttons[i].second(x, y, "");
        });
    	this->buttons.push_back(button);
        maxWidth = max(maxWidth, button->sizeHint().width());
    }

    maxWidth += popupWidthPadding;

    for(int i = 0; i < this->buttons.size(); i++) {
    	this->buttons[i]->setGeometry(0, titleHeight + subtitleHeight + 14 + i * 30, maxWidth + 20, 30);
    }
    int height = titleHeight + subtitleHeight + 14 + this->buttons.size() * 30;
    background->setGeometry(x, y, max(minPopupWidth, maxWidth + 20), height);
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

spellcheck::spellcheck(QWidget *parent) : QMainWindow(parent) {
    setMinimumSize(400, 300);

    bottomBar = new QPushButton(this);
    bottomBar->setStyleSheet(style::bottomBar);

    background = new QPushButton(this);
    background->setGeometry(0, 0, 400, 300 - bottomBarHeight);
    background->setStyleSheet(style::background);
    connect(background, &QPushButton::clicked, this, &spellcheck::addUntitledFile);

    textEdit = new QTextEdit(this);
    connect(textEdit, &QTextEdit::textChanged, this, &spellcheck::onTextChanged);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this), &QShortcut::activated, this, &spellcheck::saveFile);
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

    dictionaryButton = new QPushButton((currentDictionary != -1 ? QString::fromStdString(getFileName(dictionaries[currentDictionary].path, false)) : "Dictionary"), this);
    dictionaryButton->setStyleSheet(style::bottomBarButton);
    connect(dictionaryButton, &QPushButton::clicked, this, [this]() {
        function<void(int, int, QString)> testFunction = [this](int x, int y, QString str) {
        	qDebug() << "Test function called";
        };
        popup = new Popup(this, 0, 0, "Title", "This is a longer subtitle", {{"Button1", testFunction}, {"Button2", testFunction}, {"Button3", testFunction}});
    });
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

    focusedFile->detectErrors(textEdit->toPlainText());
    focusedFile->errors.size() < 100 ? underlineErrors() : underlineErrorsLater();
}

void spellcheck::resizeEvent(QResizeEvent* event) {
    qDebug() << "Resized!";
    int x = event->size().width();
    int y = event->size().height();
    textEdit->setGeometry(0, fileButtonHeight, x, y - fileButtonHeight - bottomBarHeight);
    background->setGeometry(0, 0, x, y - bottomBarHeight);

    bottomBar->setGeometry(0, y - bottomBarHeight - 1, x, bottomBarHeight + 2);

    int dictionaryButtonWidth = dictionaryButton->sizeHint().width();
    dictionaryButton->setGeometry(x - dictionaryButtonWidth, y - bottomBarHeight, dictionaryButtonWidth, bottomBarHeight);

    if(popup != nullptr) {
    	popup->closeButton->setGeometry(0, 0, x, y);
    }
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

void spellcheck::restoreLastSession() {
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

    underlineErrors();
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
    ofstream file("data/lastsession.txt");
	string lastSession = "";
	for(FileTab &tab : fileTabs) {
    	lastSession += tab.path + "|";
    }
	file << lastSession;
	file.close();
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

