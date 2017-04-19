#include <QApplication>
#include "Project.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

int main(int argc, char **argv)
{
//    MarkovModel::test();
//    return 0;

    QApplication app(argc, argv);
    
    KineticModelBuilder::Project project;
    project.newMarkovModel();
    project.newStimulusClampProtocol();
//    project.dump(std::cout);
    
//    MarkovModel::MarkovModel model;
//    MarkovModel::MarkovModelViewer viewer;
//    viewer.setModel(&model);
//    viewer.show();
//    
//    StimulusClampProtocol::StimulusClampProtocol protocol;
//    
//    StimulusClampProtocol::StimulusClampProtocolSimulator sim;
//    sim.model = &model;
//    sim.protocols.push_back(&protocol);
//    sim.options["Method"] = "Eigen Solver";
//    try {
//        sim.init();
//        sim.run();
//    } catch(std::runtime_error &e) {
//        std::cout << e.what() << std::endl;
//    } catch(...) {
//        std::cout << "Unknown error." << std::endl;
//    }
//    
//    StimulusClampProtocol::StimulusClampProtocolPlot plot;
//    plot.plotProtocol(&protocol, sim.stateNames);
//    plot.show();

    int status = app.exec();
//    project.dump(std::cout);
    return status;
}
