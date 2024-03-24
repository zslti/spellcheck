#include "spellcheck.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    spellcheck w;
    w.show();
    return a.exec();
}
