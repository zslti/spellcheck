#pragma once
#include "spellcheck.h"
#include "utils.h"

using namespace std;

class spellcheck;

extern QString popupSelectedText;
class Popup {
public:
    class Button {
    public:
        QString text;
        function<void()> onClick;
        bool isHighlighted;
        bool hasTopSeparator;
        bool hasBottomSeparator;
    };

    QPushButton* background;
    QLabel* title;
    QLabel* subtitle;
    QPushButton* closeButton;
    vector<QPushButton*> buttons;
    int selectedIndex;
    bool isOperableWithArrowKeys;

    Popup(spellcheck* parent, int x, int y, QString title, QString subtitle, vector<Button> buttons, int buttonHeight, bool isOperableWithArrowKeys);
    ~Popup();
};