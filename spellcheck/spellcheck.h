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
    void wheelEvent(QWheelEvent* event);
    void createFileTabs(vector<FileTab>&);
    vector<string> getFilesFromLastSession();
    void saveCurrentSession();
    void restoreLastSession();
    void focusFile(int fileID);
    void closeFile(int fileID);
    void saveFile();
    void addFile();
    void closeEvent(QCloseEvent *event);
private:
    Ui::spellcheckClass ui;
    QTextEdit* textEdit;
    QPushButton* addFileButton;
    vector<FileTab> fileTabs;
    FileTab* focusedFile = nullptr;
};

namespace style {
    const QString accentColor = "#ff0000";
    const QString fileButtonFocused =
        "color: #f0f0f0;\
        background-color: #262626;\
        text-align: left;\
        padding-left: 8px;\
        border-style: solid;\
        border-color: #202020;\
        border-bottom-color: " + accentColor + ";\
        border-width: 1px";
    const QString fileButton =
        "color: #f0f0f0;\
        background-color: #262626;\
        text-align: left;\
        padding-left: 8px;\
        border-style: solid;\
        border-color: #202020;\
        border-bottom-color: #555555;\
        border-width: 1px";

    const QString fileCloseButton = "background: transparent";
    const QString fileAddButton = 
        "background: transparent;\
        color: " + accentColor + ";\
        font-size: 20px";
}