#pragma once
#include "errordetection.h"
#include "spellcheck.h"

using namespace std;

class Error;

class FileTab {
public:
    QPushButton* button;
    QPushButton* closeButton;
    string path;
    string text;
    int id;
    bool saved;
    vector<Error> errors;
    int errorCount;
    int errorDetectionTime;
    int suggestionsTime;

    void detectErrors(QString);

    FileTab(QPushButton* button, QPushButton* closeButton, string path, string text, int id, bool saved);
    FileTab(string path) : FileTab(nullptr, nullptr, path, "", -1, true) {}
    
    void destroy();
};
