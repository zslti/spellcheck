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
#include "errordetection.h"
#include <QLabel>
#include <functional>
#include <tuple>

using namespace std;

class Error;
class spellcheck;

const int fileButtonWidth = 120;
const int fileButtonHeight = 30;
const int bottomBarHeight = 20;
const int minPopupWidth = 100;
const int popupWidthPadding = 70;

class FileTab {
public:
    QPushButton* button;
    QPushButton* closeButton;
    string path;
    string text;
    int id;
    bool saved;
    vector<Error> errors;

    void detectErrors(QString);

    FileTab(QPushButton* button, QPushButton* closeButton, string path, string text, int id, bool saved);
    FileTab(string path) : FileTab(nullptr, nullptr, path, "", -1, true) {}
    
    void destroy();
};

class Popup {
public:
    class Button {
    public:
        QString text;
        function<void(int, int, QString)> onClick;
        bool isHighlighted;
        bool hasTopSeparator;
        bool hasBottomSeparator;
    };

    QPushButton* background;
    QLabel* title;
    QLabel* subtitle;
    QPushButton* closeButton;
    vector<QPushButton*> buttons;

    Popup(spellcheck* parent, int x, int y, QString title, QString subtitle, vector<Button> buttons);
    ~Popup();
};

class spellcheck : public QMainWindow
{
    Q_OBJECT
public:
    spellcheck(QWidget *parent = nullptr);
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
    void underlineErrors();
    void underlineErrorsLater();
    void destroyPopup();
    QString getText();
    void updateBottomBarGeometry();
    void detectLanguage();
private:
    Ui::spellcheckClass ui;
    QTextEdit* textEdit;
    QPushButton* background;
    QPushButton* bottomBar;
    QPushButton* addFileButton;
    QPushButton* dictionaryButton = nullptr;
    vector<FileTab> fileTabs;
    FileTab* focusedFile = nullptr;
    Popup* popup = nullptr;
    int fileTabScrollValue = 0;
};

namespace style {
    const QString accentColor = "#77c2ff";
    const QString highlightColor = "#62788a";
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
    const QString bottomBarButton =
        "QPushButton {\
            color: #999999;\
            background-color: #262626;\
            border-style: solid;\
            border-color: #262626;\
            border-bottom-color: #555555;\
            border-width: 1px;\
            padding-left: 8px;\
            padding-right: 8px;\
        }\
        QPushButton:hover {\
            background-color: #333333;\
            border-bottom-color: " + accentColor + ";\
        }";
    const QString bottomBar = "background-color: #262626;";
    const QString fileCloseButton = "background: transparent";
    const QString fileAddButton = 
        "background: transparent;\
        color: " + accentColor + ";\
        font-size: 20px";
    const QString background = "background: transparent";
    const QString popupBackground = "background-color: #262626; border: none";
    const QString popupTitle = "color: #f0f0f0; font-size: 14px";
    const QString popupSubtitle = "color: #999999; font-size: 12px";
    const QString popupButton =
        "QPushButton {\
            color: #999999;\
            background-color: #262626;\
            border-style: solid;\
            border-color: #262626;\
            border-width: 2px;\
            text-align: left;\
            padding-left: 10px;\
        }\
        QPushButton:hover {\
            background-color: #333333;\
            border-left-color: " + accentColor + ";\
        }";
    const QString popupButtonHighlighted =
        "QPushButton {\
            color: #f0f0f0;\
            background-color: #333333;\
            border-style: solid;\
            border-color: #333333;\
            border-width: 2px;\
            text-align: left;\
            padding-left: 10px;\
            border-left-color: " + accentColor + ";\
        }";
    const QString popupButtonWithTopSeparator =
        "QPushButton {\
            border-style: solid;\
            border-width: 2px;\
            text-align: left;\
            padding-left: 10px;\
			border-top-color: #363636;\
        }";
    const QString popupButtonWithBottomSeparator =
        "QPushButton {\
            border-style: solid;\
            border-width: 2px;\
            text-align: left;\
            padding-left: 10px;\
			border-bottom-color: #363636;\
        }";
}