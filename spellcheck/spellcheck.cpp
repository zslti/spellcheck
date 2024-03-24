#include "spellcheck.h"

bool justSwithedFile = false;

spellcheck::spellcheck(QWidget *parent) : QMainWindow(parent) {
    setMinimumSize(400, 300);

    textEdit = new QTextEdit(this);
    connect(textEdit, &QTextEdit::textChanged, this, &spellcheck::onTextChanged);
    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this), &QShortcut::activated, this, &spellcheck::saveFile);

    addFileButton = new QPushButton("+", this);
    addFileButton->setGeometry(0, 0, fileButtonHeight + 1, fileButtonHeight + 1);
    addFileButton->setStyleSheet(style::fileAddButton);
    connect(addFileButton, &QPushButton::clicked, this, &spellcheck::addFile);

    restoreLastSession();
}

spellcheck::~spellcheck() {}

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

    QTextCursor cursor = textEdit->textCursor();

    // töroljuk az előző hibákat
    cursor.select(QTextCursor::Document);
    textEdit->blockSignals(true);
    cursor.setCharFormat(QTextCharFormat());
    textEdit->blockSignals(false);
    cursor.clearSelection();

    cursor.movePosition(QTextCursor::Start);
    QTextCharFormat format;
    format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    format.setUnderlineColor(Qt::red);
  
    int index = text.indexOf("world");
    while(index != -1) {
		cursor.setPosition(index);
		cursor.setPosition(index + 5, QTextCursor::KeepAnchor);
        textEdit->blockSignals(true);
        cursor.setCharFormat(format);
        textEdit->blockSignals(false);
		index = text.indexOf("world", index + 5);
	}
}

void spellcheck::resizeEvent(QResizeEvent* event) {
    qDebug() << "Resized!";
    textEdit->setGeometry(0, fileButtonHeight, event->size().width(), event->size().height() - fileButtonHeight);
}

void spellcheck::wheelEvent(QWheelEvent* event) {   
    QPoint cursor = QWidget::mapFromGlobal(QCursor::pos());

    if(cursor.y() < fileButtonHeight) { // ha a file bar-on van a kurzor
		qDebug() << "Scrolling on file bar";
        qDebug() << event->angleDelta().x();
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
		QPushButton* button = new QPushButton(QString::fromStdString(tabs[i].getFileName()), this);
		button->setGeometry(fileTabs.size() * (fileButtonWidth - 1), 0, fileButtonWidth, fileButtonHeight + 1);
        button->setVisible(true);
        button->setStyleSheet(style::fileButton);
        connect(button, &QPushButton::clicked, this, [this, id]() {focusFile(id);});

        // file bezáró gombok létrehozása
        QPushButton* closeButton = new QPushButton("X", this);
        closeButton->setGeometry(fileTabs.size() * (fileButtonWidth - 1) + fileButtonWidth - 20, 0, 20, fileButtonHeight + 1);
        closeButton->setStyleSheet(style::fileCloseButton);
        closeButton->setVisible(true);
        connect(closeButton, &QPushButton::clicked, this, [this, id]() {closeFile(id);});

        // file tartalmának betöltése
        ifstream file(tabs[i].path);
        stringstream buffer;
        buffer << file.rdbuf();
        file.close();

		fileTabs.push_back({button, closeButton, tabs[i].path, buffer.str(), id, tabs[i].saved});
	}
    if(focusedFile == nullptr) focusFile(fileTabs[0].id);

    // az új file gombot a fileok végéhez teszük
    addFileButton->setGeometry(fileTabs.size() * (fileButtonWidth - 1), 0, fileButtonHeight + 1, fileButtonHeight + 1);
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
	ifstream file("lastsession.txt");
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
	vector<string> files = getFilesFromLastSession();
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
}

void spellcheck::saveFile() {
	if(focusedFile == nullptr) return;
	ofstream file(focusedFile->path);
	file << textEdit->toPlainText().toStdString();
	file.close();
	focusedFile->saved = true;
	focusedFile->button->setText(QString::fromStdString(focusedFile->getFileName()));
}

void spellcheck::closeFile(int fileID) {
	// kitöröljük a jelenlegi file gombot
    int i = 0;
    for(i; i < fileTabs.size(); i++) {
        if(fileTabs[i].id == fileID) {
            // ha a törölt file volt a fókuszban, akkor az elsőt állítjuk fókuszba
            bool wasFocused = (focusedFile == &fileTabs[i] && fileTabs.size() > 1);

            delete fileTabs[i].button;
            delete fileTabs[i].closeButton;
            fileTabs.erase(fileTabs.begin() + i);

            if(wasFocused) {
                focusedFile = nullptr;
                focusFile(fileTabs[0].id);
            }
			break;
		}
	}

    // balra toljuk a többi file gombot
    for(i; i < fileTabs.size(); i++) {
        fileTabs[i].button->setGeometry(i * (fileButtonWidth - 1), 0, fileButtonWidth, fileButtonHeight + 1);
        fileTabs[i].closeButton->setGeometry(i * (fileButtonWidth - 1) + fileButtonWidth - 20, 0, 20, fileButtonHeight + 1);
    }

    // toljuk balra az új file gombot
    addFileButton->setGeometry(fileTabs.size() * (fileButtonWidth - 1), 0, fileButtonHeight + 1, fileButtonHeight + 1);
}   

void spellcheck::addFile() {
	QString path = QFileDialog::getOpenFileName(this, "Open file");
	if(path.isEmpty()) return;

	FileTab tab = {nullptr, nullptr, path.toStdString(), "", 0, true};
    vector<FileTab> tabs = {tab};
    createFileTabs(tabs);
    focusFile(fileTabs[fileTabs.size() - 1].id);
}

string FileTab::getFileName() {
    string fileName = this->path;

    size_t pos = fileName.find_last_of("/\\");
    if(pos != string::npos) {
    	fileName = fileName.substr(pos + 1);
    }

    if(fileName.size() > 15) {
		fileName = fileName.substr(0, 15) + "...";
	}
	return fileName;
}

void spellcheck::saveCurrentSession() {
    ofstream file("lastsession.txt");
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