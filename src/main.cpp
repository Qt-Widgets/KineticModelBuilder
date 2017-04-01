#include <QApplication>
#include "MarkovModel.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

int main(int /* argc */, char ** /* argv */)
{
    MarkovModel::test();
    return 0;

//    QApplication app(argc, argv);

//    int status = app.exec();
//    return status;
}
