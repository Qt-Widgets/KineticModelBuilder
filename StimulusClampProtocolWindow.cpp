/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "StimulusClampProtocolWindow.h"
#include "StimulusClampProtocol.h"
#include "StimulusClampProtocolPropertyEditor.h"
#include "StimulusClampProtocolPlot.h"
#include "Project.h"
#include <QApplication>
#include <QDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

namespace StimulusClampProtocol
{
    StimulusClampProtocolWindow::StimulusClampProtocolWindow(StimulusClampProtocol* protocol, QWidget *parent) :
    QMainWindow(parent),
    _protocol(protocol)
    {
        // Project is parent of model.
        _project = _protocol && _protocol->parent() ? qobject_cast<KineticModelBuilder::Project*>(_protocol->parent()) : 0;
        
        // Plots grid.
        QWidget *plots = new QWidget();
        _plotGrid = new QGridLayout(plots);
        _plotGrid->setContentsMargins(1, 1, 1, 1);
        _plotGrid->setSpacing(1);
        setCentralWidget(plots);
        plots->setFocusPolicy(Qt::StrongFocus);
        
        // Window title.
        setWindowTitle(_protocol->name());
        if(_protocol)
            connect(_protocol, SIGNAL(objectNameChanged(QString)), this, SLOT(setWindowTitle(QString)));
        
        // Should NOT happen!!!
        if(!_project)
            return;
        
//        connect(this, SIGNAL(plotGridResized()), _project, SLOT(updateWindows()));
        
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
        fileMenu->addAction("Save", _protocol, SLOT(save()), QKeySequence::Save);
        fileMenu->addAction("Save As", _protocol, SLOT(saveAs()), QKeySequence::SaveAs);
        fileMenu->addSeparator();
        fileMenu->addAction("Save Project", _project, SLOT(saveAs()));
        fileMenu->addSeparator();
        fileMenu->addAction("Quit", qApp, SLOT(quit()), QKeySequence::Quit);
        menuBar()->addMenu(fileMenu);
        
        // Protocol menu.
        QMenu *protocolMenu = new QMenu("Stimulus Clamp Protocol");
        protocolMenu->addAction("Protocol Parameters", this, SLOT(editProtocol()), QKeySequence::Italic);
        protocolMenu->addSeparator();
        protocolMenu->addAction("Simulation Cost", this, SLOT(showCost()));
        menuBar()->addMenu(protocolMenu);
        
        // Windows menu.
        QMenu *windowMenu = new QMenu("Window");
        windowMenu->addAction("Tile Windows", _project, SLOT(tileWindows()));
        windowMenu->addSeparator();
        windowMenu->addAction("Window Options", _project, SLOT(editWindowOptions()));
        menuBar()->addMenu(windowMenu);
        
        // Toolbar.
        _visibleVariableSetsBox = new QMultipleIndexSpinBox();
        _visibleVariableSetsBox->setMinimumWidth(75);
        _visibleVariableSetsBox->setToolTip("Visible model variable sets.");
        connect(_visibleVariableSetsBox, SIGNAL(valueChanged()), this, SLOT(replot()));
        
        _visibleRowsBox = new QMultipleIndexSpinBox();
        _visibleRowsBox->setMinimumWidth(75);
        _visibleRowsBox->setToolTip("Visible rows of conditions matrix.");
        connect(_visibleRowsBox, SIGNAL(valueChanged()), this, SLOT(replot()));
        
        _visibleColumnsBox = new QMultipleIndexSpinBox();
        _visibleColumnsBox->setMinimumWidth(75);
        _visibleColumnsBox->setToolTip("Visible columns of conditions matrix.");
        connect(_visibleColumnsBox, SIGNAL(valueChanged()), this, SLOT(replot()));
        
        _visibleEventChainsBox = new QMultipleIndexSpinBox();
        _visibleEventChainsBox->setValue(0);
        _visibleEventChainsBox->setMinimumWidth(75);
        _visibleEventChainsBox->setToolTip("Visible Monte Carlo event chains.");
        connect(_visibleEventChainsBox, SIGNAL(valueChanged()), this, SLOT(replot()));
        
        QSpinBox *plotRowsBox = new QSpinBox();
        plotRowsBox->setValue(1);
        plotRowsBox->setRange(1, 99);
        plotRowsBox->setSuffix(" row(s)");
        connect(plotRowsBox, SIGNAL(valueChanged(int)), this, SLOT(setPlotRows(int)));
        
        QSpinBox *plotColumnsBox = new QSpinBox();
        plotColumnsBox->setValue(1);
        plotColumnsBox->setRange(1, 99);
        plotColumnsBox->setSuffix(" column(s)");
        connect(plotColumnsBox, SIGNAL(valueChanged(int)), this, SLOT(setPlotColumns(int)));
        
        QToolButton *plotGridButton = new QToolButton();
        plotGridButton->setPopupMode(QToolButton::InstantPopup);
        plotGridButton->setText("plots");
        plotGridButton->setToolTip("Plots grid.");
        //plotGridButton->setIcon(QIcon("../Resources/icons/playsettings.png"));
        QWidget *plotGridUi = new QWidget;
        QHBoxLayout *plotGridUiLayout = new QHBoxLayout(plotGridUi);
        plotGridUiLayout->setContentsMargins(1, 1, 1, 1);
        plotGridUiLayout->setSpacing(1);
        plotGridUiLayout->addWidget(plotRowsBox);
        plotGridUiLayout->addWidget(plotColumnsBox);
        QMenu *plotGridButtonMenu = new QMenu;
        QWidgetAction *plotGridButtonAction = new QWidgetAction(plotGridButtonMenu);
        plotGridButtonAction->setDefaultWidget(plotGridUi);
        plotGridButtonMenu->addAction(plotGridButtonAction);
        plotGridButton->setMenu(plotGridButtonMenu);
        
        QToolButton *paramsButton = new QToolButton();
        paramsButton->setText("params");
        paramsButton->setToolTip("Edit protocol parameters.");
        //paramsButton->setIcon(QIcon("../Resources/icons/playsettings.png"));
        connect(paramsButton, SIGNAL(released()), this, SLOT(editProtocol()));
        
        QToolBar *toolBar = addToolBar("");
        _visibleVariableSetsBoxAction = toolBar->addWidget(_visibleVariableSetsBox);
        _visibleRowsBoxAction = toolBar->addWidget(_visibleRowsBox);
        _visibleColumnsBoxAction = toolBar->addWidget(_visibleColumnsBox);
        _visibleEventChainsBoxAction = toolBar->addWidget(_visibleEventChainsBox);
        toolBar->addWidget(plotGridButton);
        toolBar->addWidget(paramsButton);
        
        _visibleVariableSetsBoxAction->setVisible(false);
        _visibleRowsBoxAction->setVisible(false);
        _visibleColumnsBoxAction->setVisible(false);
        _visibleEventChainsBoxAction->setVisible(false);
        
        resizePlotGrid(1, 1);
    }
    
    void StimulusClampProtocolWindow::resizePlotGrid(int rows, int cols)
    {
        foreach(StimulusClampProtocolPlot *plot, _plots)
            _plotGrid->removeWidget(plot);
        size_t count = 0;
        for(int row = 0; row < rows; ++row) {
            for(int col = 0; col < cols; ++col) {
                if(_plots.size() <= count) {
                    StimulusClampProtocolPlot *plot = new StimulusClampProtocolPlot();
                    connect(plot, SIGNAL(optionsChanged()), this, SLOT(checkIfWeNeedToShowTheEventChainUi()));
                    plot->setProtocol(_protocol);
                    plot->setVisibleVariableSetIndexes(_visibleVariableSetsBox->text());
                    plot->setVisibleRows(_visibleRowsBox->text());
                    plot->setVisibleColumns(_visibleColumnsBox->text());
                    plot->setVisibleEventChains(_visibleEventChainsBox->text());
                    plot->plotProtocol();
                    _plots.push_back(plot);
                }
                _plotGrid->addWidget(_plots.at(count), row, col);
                ++count;
            }
        }
        while(_plots.size() > count) {
            delete _plots.back();
            _plots.pop_back();
        }
        _plotRows = rows;
        _plotColumns = cols;
        emit plotGridResized();
    }
    
    void StimulusClampProtocolWindow::replot()
    {
        int rows = _protocol->simulations.size();
        int cols = rows ? _protocol->simulations[0].size() : 0;
        int sets = cols ? _protocol->simulations[0][0].waveforms.size() : 0;
        int chains = sets ? (_protocol->simulations[0][0].events.size() ? _protocol->simulations[0][0].events.at(0).size() : 0) : 0;
        _visibleVariableSetsBox->setRange(0, sets - 1);
        _visibleRowsBox->setRange(0, rows - 1);
        _visibleColumnsBox->setRange(0, cols - 1);
        _visibleEventChainsBox->setRange(0, chains - 1);
        _visibleVariableSetsBoxAction->setVisible(sets > 1);
        _visibleRowsBoxAction->setVisible(rows > 1);
        _visibleColumnsBoxAction->setVisible(cols > 1);
        checkIfWeNeedToShowTheEventChainUi();
        foreach(StimulusClampProtocolPlot *plot, _plots) {
            plot->setDrawingEnabled(false);
            plot->setVisibleVariableSetIndexes(_visibleVariableSetsBox->text());
            plot->setVisibleRows(_visibleRowsBox->text());
            plot->setVisibleColumns(_visibleColumnsBox->text());
            plot->setVisibleEventChains(_visibleEventChainsBox->text());
            plot->setDrawingEnabled(true);
            plot->plotProtocol();
        }
    }
    
    void StimulusClampProtocolWindow::editProtocol()
    {
        if(!_protocol) return;
        QDialog dialog(this);
        dialog.setModal(true);
        dialog.setWindowTitle("Protocol Parameters");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(new StimulusClampProtocolPropertyEditor(_protocol));
        dialog.show();
        QPoint topLeft = mapToGlobal(QPoint(0, 0));
        dialog.setGeometry(topLeft.x(), topLeft.y(), width(), height());
        dialog.exec();
    }
    
    void StimulusClampProtocolWindow::showMaxProbabilityError()
    {
        if(!_protocol) return;
        double Perror = 0;
        for(size_t row = 0; row < _protocol->simulations.size(); ++row) {
            for(size_t col = 0; col < _protocol->simulations[row].size(); ++col) {
                Simulation &sim = _protocol->simulations[row][col];
                double simPerror = sim.maxProbabilityError();
                if(simPerror > Perror)
                    Perror = simPerror;
            }
        }
        statusBar()->showMessage("Perror <= " + QString::number(Perror));
    }
    
    void StimulusClampProtocolWindow::showCost()
    {
        if(!_protocol) return;
        double cost = _protocol->cost();
//        statusBar()->showMessage("Cost = " + QString::number(cost));
        QString messageTitle = "Protocol Cost";
        QString message = "Cost for '" + _protocol->name() + "' = " + QString::number(cost);
        QMessageBox::information(this, messageTitle, message);
    }
    
    void StimulusClampProtocolWindow::checkIfWeNeedToShowTheEventChainUi()
    {
        foreach(StimulusClampProtocolPlot *plot, _plots) {
            if(plot->showEventChains()) {
                _visibleEventChainsBoxAction->setVisible(true);
                return;
            }
        }
        _visibleEventChainsBoxAction->setVisible(false);
    }
    
} // StimulusClampProtocol
