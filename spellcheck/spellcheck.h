#pragma once

#include <QTextEdit>
#include <QTimer>
#include <QPushButton>
#include <QShortcut>
#include <QFileDialog>
#include <QCloseEvent>
#include <QLabel>
#include "ui_spellcheck.h"
#include "errordetection.h"
#include "filetab.h"
#include "popup.h"
#include "utils.h"

using namespace std;

class Error;
class spellcheck;
class Settings;
class Popup;

const int fileButtonWidth = 120;
const int fileButtonHeight = 30;
const int bottomBarHeight = 20;
const int minPopupWidth = 100;
const int popupWidthPadding = 70;
extern Settings settings;
extern bool acceptingPopupSelection;
extern bool justSwitchedFile;

class Popup;
class FileTab;

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
    void addUntitledFile();
    void underlineErrors();
    void underlineErrorsLater();
    void destroyPopup();
    QString getText();
    void updateBottomBarGeometry();
    void detectLanguage();
    void onCursorChanged();
    void handleArrow(bool direction);
    void handleArrowDown();
    void handleArrowUp();
    void acceptPopupSelection();
    void restoreDefaultSettings();
    void insertSpaceWithoutAutoCorrect();
private:
    Ui::spellcheckClass ui;
    QTextEdit* textEdit;
    QPushButton* background;
    QPushButton* bottomBar;
    QPushButton* addFileButton;
    QPushButton* dictionaryButton = nullptr;
    QPushButton* autoCorrectButton = nullptr;
    QPushButton* errorsButton = nullptr;
    QPushButton* suggestionsButton = nullptr;
    vector<FileTab> fileTabs;
    Popup* popup = nullptr;
    int fileTabScrollValue = 0;
public:
    static FileTab* focusedFile;
};

class ErrorTypeSetting {
public:
    QString name;
    bool enabled;

    ErrorTypeSetting(QString name = "", bool enabled = true) : name(name), enabled(enabled) {}
};

struct Settings {
public:
    bool autoCorrect;
    bool areSuggestionsEnabled;
    map<int, ErrorTypeSetting> errorTypes;

    Settings();
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
            border-right-color: transparent;\
            border-left-color: transparent;\
            border-width: 1px;\
            padding-left: 8px;\
            padding-right: 8px;\
        }\
        QPushButton:hover {\
            background-color: #333333;\
            border-bottom-color: " + accentColor + ";\
        }";
    const QString bottomBar = 
        "background-color: #262626;\
         border-style: solid;\
         border-bottom-color: #555555;\
         border-width: 1px";
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