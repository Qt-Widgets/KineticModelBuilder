#include <QApplication>
#include "Project.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    
    KineticModelBuilder::Project project;
    project.newMarkovModel();
    project.newStimulusClampProtocol();

    int status = app.exec();
    return status;
}
