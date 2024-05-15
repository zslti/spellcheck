#include "popup.h"

using namespace std;

Popup::Popup(spellcheck* parent, int x, int y, QString title, QString subtitle, vector<Popup::Button> buttons, int buttonHeight = 30, bool isOperableWithArrowKeys = false) {
    this->selectedIndex = 0;
    
    closeButton = new QPushButton(parent);
    QObject::connect(closeButton, &QPushButton::clicked, [=]() {parent->destroyPopup();});

    background = new QPushButton(parent);
    background->setStyleSheet(style::popupBackground);
    background->show();

    this->isOperableWithArrowKeys = isOperableWithArrowKeys;
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
    x = keepBetween(x, 0, parent->size().width() - maxWidth);
    y = keepBetween(y, fileButtonHeight, parent->size().height() - height - bottomBarHeight);

    background->setGeometry(x, y, max(minPopupWidth, maxWidth), height);
}

Popup::~Popup() {
    delete title;
    delete subtitle;
    for(QPushButton* button : buttons) {
        delete button;
    }
    delete closeButton;
    delete background;
}

void spellcheck::destroyPopup() {
    if(this->popup == nullptr) return;
    delete this->popup;
    this->popup = nullptr;
}