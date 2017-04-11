#include <QApplication>
#include "MarkovModel.h"
#include "MarkovModelViewer.h"
#include "StimulusClampProtocol.h"
#include "StimulusClampProtocolPlot.h"
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
    
    StimulusClampProtocol::StimulusClampProtocol protocol;
    
    StimulusClampProtocol::StimulusClampProtocolSimulator sim;
    sim.model = &model;
    sim.protocols.push_back(&protocol);
    sim.options["Method"] = "Eigen Solver";
    try {
        sim.init();
        sim.run();
    } catch(std::runtime_error &e) {
        std::cout << e.what() << std::endl;
    } catch(...) {
        std::cout << "Unknown error." << std::endl;
    }
    
    StimulusClampProtocol::StimulusClampProtocolPlot plot;
    plot.plotProtocol(&protocol, sim.stateNames);
    plot.show();

    int status = app.exec();
    return status;
}
