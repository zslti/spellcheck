#pragma once

#include "ui_spellcheck.h"
#include <QtWidgets/QMainWindow>
#include <QTextEdit>
#include <QTimer>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QDebug>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QPushButton>
#include <QObject>
#include <QEvent>
#include <QShortcut>
#include <QFileDialog>
#include <QKeySequence>
#include <QCloseEvent>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

const int fileButtonWidth = 120;
const int fileButtonHeight = 30;

class FileTab {
public:
    QPushButton* button;
    QPushButton* closeButton;
    string path;
    string text;
    int id;
    bool saved;

    string getFileName();
};

class spellcheck : public QMainWindow
{
    Q_OBJECT
public:
    spellcheck(QWidget *parent = nullptr);
    ~spellcheck();
public slots:
    void onTextChanged();
    void resizeEvent(QResizeEvent*);
    void wheelEvent(QWheelEvent*);
    void createFileTabs(vector<FileTab>&);
    vector<string> getFilesFromLastSession();
    void saveCurrentSession();
    void restoreLastSession();
    void focusFile(int);
    void closeFile(int);
    void saveFile();
    void addFile();
    void closeEvent(QCloseEvent*);
    string getNewUntitledFile();
    bool fileExists(string);
    void addUntitledFile();
    int keepBetween(int, int, int);
private:
    Ui::spellcheckClass ui;
    QTextEdit* textEdit;
    QPushButton* background;
    QPushButton* addFileButton;
    vector<FileTab> fileTabs;
    FileTab* focusedFile = nullptr;
    int fileTabScrollValue = 0;
};

namespace style {
    const QString accentColor = "#ff0000";
    const QString fileButtonFocused =
        "QPushButton {\
            color: #f0f0f0;\
            background-color: #262626;\
            text-align: left;\
            padding-left: 8px;\
            border-style: solid;\
            border-color: #202020;\
            border-bottom-color: " + accentColor + ";\
            border-width: 1px;\
        }\
        QPushButton:hover {\
            background-color: #333333;\
        }";
    const QString fileButton =
        "QPushButton {\
            color: #f0f0f0;\
            background-color: #262626;\
            text-align: left;\
            padding-left: 8px;\
            border-style: solid;\
            border-color: #202020;\
            border-bottom-color: #555555;\
            border-width: 1px;\
        }\
        QPushButton:hover {\
            background-color: #333333;\
        }";
    const QString fileCloseButton = "background: transparent";
    const QString fileAddButton = 
        "background: transparent;\
        color: " + accentColor + ";\
        font-size: 20px";
    const QString background = "background: transparent";
}