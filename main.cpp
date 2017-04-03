#include <QApplication>
#include "MarkovModel.h"
#include "QObjectPropertyEditor.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

int main(int argc, char **argv)
{
//    MarkovModel::test();
//    return 0;
    
    return QObjectPropertyEditor::testQObjectPropertyEditor(argc, argv);

//    QApplication app(argc, argv);
//    int status = app.exec();
//    return status;
}
