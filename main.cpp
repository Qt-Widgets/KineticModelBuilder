#include <QApplication>
#include "MarkovModel.h"
#include "MarkovModelViewer.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

int main(int argc, char **argv)
{
//    MarkovModel::test();
//    return 0;

    QApplication app(argc, argv);
    MarkovModel::MarkovModel model;
    MarkovModel::MarkovModelViewer viewer;
    viewer.setModel(&model);
    viewer.show();
    int status = app.exec();
    return status;
}
