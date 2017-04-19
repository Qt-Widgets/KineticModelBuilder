/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "MarkovModelWindow.h"
#include "MarkovModel.h"
#include "MarkovModelViewer.h"
#include "Project.h"
#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

namespace MarkovModel
{
    MarkovModelWindow::MarkovModelWindow(MarkovModel* model, QWidget *parent) :
    QMainWindow(parent),
    _model(model)
    {
        // Project is parent of model.
        _project = _model && _model->parent() ? qobject_cast<KineticModelBuilder::Project*>(_model->parent()) : 0;
        
        // Viewer.
        _viewer = new MarkovModelViewer();
        _viewer->setModel(_model);
        setCentralWidget(_viewer);
        _viewer->setFocusPolicy(Qt::StrongFocus);
        
        // Window title.
        setWindowTitle(_model->name());
        if(_model)
            connect(_model, SIGNAL(objectNameChanged(QString)), this, SLOT(setWindowTitle(QString)));
        
        // Should NOT happen!!!
        if(!_project)
            return;
        
        // File menu.
        QMenu *fileMenu = new QMenu("File");
        fileMenu->addAction("About", _project, SLOT(about()));
        fileMenu->addSeparator();
        fileMenu->addMenu(_project->newMenu());
        fileMenu->addSeparator();
        fileMenu->addAction("Open", _project, SLOT(open()), QKeySequence::Open);
        fileMenu->addSeparator();
        fileMenu->addAction("Close", this, SLOT(close()), QKeySequence::Close);
        fileMenu->addSeparator();
        fileMenu->addAction("Save", _model, SLOT(save()), QKeySequence::Save);
        fileMenu->addAction("Save As", _model, SLOT(saveAs()), QKeySequence::SaveAs);
        fileMenu->addSeparator();
        fileMenu->addAction("Save Project", _project, SLOT(saveAs()));
        fileMenu->addSeparator();
        fileMenu->addAction("Quit", qApp, SLOT(quit()), QKeySequence::Quit);
        menuBar()->addMenu(fileMenu);
        
        // Model menu.
        QMenu *modelMenu = _viewer->editMenu("Markov Model");
        menuBar()->addMenu(modelMenu);
        
        // Simulation menu.
        QMenu *simulationMenu = new QMenu("Simulation");
        simulationMenu->addAction("Run Simulation", _project, SLOT(simulate()), QKeySequence::Refresh);
        simulationMenu->addSeparator();
        simulationMenu->addAction("Optimize", _project, SLOT(optimize()));
        simulationMenu->addSeparator();
        simulationMenu->addAction("Simulation Options", _project, SLOT(editSimulationOptions()));
        menuBar()->addMenu(simulationMenu);
        
        // Windows menu.
        QMenu *windowMenu = new QMenu("Window");
        windowMenu->addAction("Tile Windows", _project, SLOT(tileWindows()));
        windowMenu->addSeparator();
        windowMenu->addAction("Window Options", _project, SLOT(editWindowOptions()));
        menuBar()->addMenu(windowMenu);
        
        // Toolbar.
        QToolButton *simButton = new QToolButton();
        simButton->setText("simulate");
        simButton->setToolTip("Run a simulation.");
        //simButton->setIcon(QIcon("../Resources/icons/playsettings.png"));
        connect(simButton, SIGNAL(released()), _project, SLOT(simulate()));
        
        QToolButton *paramsButton = new QToolButton();
        paramsButton->setText("params");
        paramsButton->setToolTip("Edit model parameters.");
        //paramsButton->setIcon(QIcon("../Resources/icons/playsettings.png"));
        connect(paramsButton, SIGNAL(released()), _viewer, SLOT(editModel()));
        
        QToolBar *toolBar = addToolBar("");
        toolBar->addWidget(simButton);
        toolBar->addWidget(paramsButton);
    }
    
} // MarkovModel
