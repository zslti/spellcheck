#include "filetab.h"

FileTab::FileTab(QPushButton* button, QPushButton* closeButton, string path, string text, int id, bool saved) : errors() {
    this->button = button;
    this->closeButton = closeButton;
    this->path = path;
    this->text = text;
    this->id = id;
    this->saved = saved;
    this->errorDetectionTime = -1;
    this->suggestionsTime = -1;
    this->errorCount = 0;
    detectErrors(QString::fromStdString(text));
}

FileTab* spellcheck::focusedFile = nullptr;

void FileTab::destroy() {
    if(button != nullptr) delete button;
    if(closeButton != nullptr) delete closeButton;
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

void spellcheck::focusFile(int fileID) {
    justSwitchedFile = true;
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
    // kitöröljük a jelenlegi file gombot
    int i = 0;
    int deletedI = -1;
    for(i; i < fileTabs.size(); i++) {
        if(fileTabs[i].id == fileID) {
            // ha a törölt file volt a fókuszban, akkor az elsőt állítjuk fókuszba
            bool wasFocused = (focusedFile == &fileTabs[i] && fileTabs.size() > 1);
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

string spellcheck::getNewUntitledFile() {
    static int counter = 0;
    string fileName = "untitled" + (counter == 0 ? "" : "-" + to_string(counter)) + ".txt";
    counter++;
    if(fileExists(fileName)) return getNewUntitledFile();
    return fileName;
 }

void spellcheck::addUntitledFile() {
    string fileName = getNewUntitledFile();
    FileTab tab(fileName);
    vector<FileTab> tabs = {tab};
    createFileTabs(tabs);
    focusedFile = nullptr;
    focusFile(fileTabs[fileTabs.size() - 1].id);
}