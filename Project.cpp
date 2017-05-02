/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "Project.h"
#include "MarkovModel.h"
#include "MarkovModelWindow.h"
#include "QObjectPropertyEditor.h"
#include "StimulusClampProtocol.h"
#include "StimulusClampProtocolWindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QDialog>
#include <QErrorMessage>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonDocument>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

namespace KineticModelBuilder
{
    QObjectPropertyTreeSerializer::ObjectFactory getObjectFactory()
    {
        QObjectPropertyTreeSerializer::ObjectFactory factory;
        factory.registerCreator("MarkovModel::MarkovModel", factory.defaultCreator<MarkovModel::MarkovModel>);
        factory.registerCreator("MarkovModel::Variable", factory.defaultCreator<MarkovModel::Variable>);
        factory.registerCreator("MarkovModel::State", factory.defaultCreator<MarkovModel::State>);
        factory.registerCreator("MarkovModel::Transition", factory.defaultCreator<MarkovModel::Transition>);
        factory.registerCreator("MarkovModel::BinaryElement", factory.defaultCreator<MarkovModel::BinaryElement>);
        factory.registerCreator("MarkovModel::Interaction", factory.defaultCreator<MarkovModel::Interaction>);
        factory.registerCreator("MarkovModel::StateGroup", factory.defaultCreator<MarkovModel::StateGroup>);
        factory.registerCreator("StimulusClampProtocol::StimulusClampProtocol", factory.defaultCreator<StimulusClampProtocol::StimulusClampProtocol>);
        factory.registerCreator("StimulusClampProtocol::Stimulus", factory.defaultCreator<StimulusClampProtocol::Stimulus>);
        factory.registerCreator("StimulusClampProtocol::Waveform", factory.defaultCreator<StimulusClampProtocol::Waveform>);
        factory.registerCreator("StimulusClampProtocol::SimulationsSummary", factory.defaultCreator<StimulusClampProtocol::SimulationsSummary>);
        factory.registerCreator("StimulusClampProtocol::ReferenceData", factory.defaultCreator<StimulusClampProtocol::ReferenceData>);
        return factory;
    }
    QObjectPropertyTreeSerializer::ObjectFactory Project::objectFactory = getObjectFactory();
    
    QMenu* Project::newMenu()
    {
        QMenu *menu = new QMenu("New");
        menu->addAction("Markov Model", this, SLOT(newMarkovModel()));
        menu->addAction("Stimulus Clamp Protocol", this, SLOT(newStimulusClampProtocol()));
        return menu;
    }
    
#ifdef DEBUG
    void Project::dump(std::ostream &out)
    {
        QVariantMap data = QObjectPropertyTreeSerializer::serialize(this, -1, true, false);
        out << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented).toStdString();
    }
#endif
    
    void Project::about()
    {
        QString title = "About Kinetic Model Builder";
        QString html = "<font style='font-weight:normal;'>"
        "<p>"
        "<b>Kinetic Model Builder " + version() + "</b>"
        "</p>"
        "<p>"
        "<b>Author</b>: Marcel Paz Goldschen-Ohm, PhD<br/>"
        "<b>Email</b>: <a href='mailto:marcel.goldschen@gmail.com'>marcel.goldschen@gmail.com</a><br/>"
        "<b>GitHub</b>: <a href='https://github.com/marcel-goldschen-ohm'>https://github.com/marcel-goldschen-ohm</a><br/>"
        "<b>LinkedIn</b>: <a href='https://www.linkedin.com/in/marcel-goldschen-ohm-543b909'>https://www.linkedin.com/in/marcel-goldschen-ohm-543b909</a>"
        "</p>"
        "<p>"
        "<b>License</b>: GPL<br/>"
        "Copyright (2017) by Marcel Paz Goldschen-Ohm"
        "</p>"
        "<p>"
        "<b>Citation</b>: If you use this software for a publication, please cite:"
        " <a href='https://dx.doi.org/10.1085/jgp.201411183'>https://dx.doi.org/10.1085/jgp.201411183</a>"
        "</p>"
        "</font>";
        QMessageBox msgBox(0);
        msgBox.setWindowTitle(title);
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(html);
        msgBox.exec();
    }
    
    void Project::newMarkovModel()
    {
        _newObjectWithUI<MarkovModel::MarkovModel, MarkovModel::MarkovModelWindow>();
        if(_autoTileWindows)
            tileWindows();
    }
    
    void Project::newStimulusClampProtocol()
    {
        _newObjectWithUI<StimulusClampProtocol::StimulusClampProtocol, StimulusClampProtocol::StimulusClampProtocolWindow>();
        if(_autoTileWindows)
            tileWindows();
    }
    
    void Project::open(QString filePath)
    {
        if(filePath.isEmpty())
            filePath = QFileDialog::getOpenFileName(0, "Open file...", _fileInfo.absoluteFilePath());
        if(filePath.isEmpty())
            return;
        QFile file(filePath);
        if(!file.open(QIODevice::Text | QIODevice::ReadOnly))
            return;
        QString buffer = file.readAll();
        _fileInfo = QFileInfo(file);
        file.close();
        QVariantMap data = QJsonDocument::fromJson(buffer.toUtf8()).toVariant().toMap();
        if(data.isEmpty())
            return;
        QList<QMainWindow*> oldWindows;
        foreach(QWidget *widget, QApplication::topLevelWidgets()) {
            if(QMainWindow *window = qobject_cast<QMainWindow*>(widget))
                oldWindows.push_back(window);
        }
        size_t numNewWindows = 0;
        QVariantMap projectData;
        foreach(const QString &key, data.keys()) {
            if(key == "MarkovModel::MarkovModel") {
                if(data[key].type() == QVariant::List) {
                    foreach(const QVariant &node, data[key].toList()) {
                        if(node.type() == QVariant::Map) {
                            _newObjectWithUI<MarkovModel::MarkovModel, MarkovModel::MarkovModelWindow>(node.toMap());
                            ++numNewWindows;
                        }
                    }
                } else if(data[key].type() == QVariant::Map) {
                    _newObjectWithUI<MarkovModel::MarkovModel, MarkovModel::MarkovModelWindow>(data[key].toMap());
                    ++numNewWindows;
                }
            } else if(key == "StimulusClampProtocol::StimulusClampProtocol") {
                if(data[key].type() == QVariant::List) {
                    foreach(const QVariant &node, data[key].toList()) {
                        if(node.type() == QVariant::Map) {
                            _newObjectWithUI<StimulusClampProtocol::StimulusClampProtocol, StimulusClampProtocol::StimulusClampProtocolWindow>(node.toMap());
                            ++numNewWindows;
                        }
                    }
                } else if(data[key].type() == QVariant::Map) {
                    _newObjectWithUI<StimulusClampProtocol::StimulusClampProtocol, StimulusClampProtocol::StimulusClampProtocolWindow>(data[key].toMap());
                    ++numNewWindows;
                }
            } else {
                projectData[key] = data[key];
            }
        }
        if(projectData.size())
            QObjectPropertyTreeSerializer::deserialize(this, projectData);
        if(numNewWindows > 1) {
            bool temp = _autoTileWindows;
            _autoTileWindows = false;
            qDeleteAll(oldWindows);
            _autoTileWindows = temp;
        }
        if(_autoTileWindows)
            tileWindows();
    }
    
    void Project::saveAs(QString filePath)
    {
        if(filePath.isEmpty())
            filePath = QFileDialog::getSaveFileName(0, "Save project...", _fileInfo.absoluteFilePath());
        if(filePath.isEmpty())
            return;
        QFile file(filePath);
        if(!file.open(QIODevice::Text | QIODevice::WriteOnly))
            return;
        QTextStream out(&file);
        QVariantMap data = QObjectPropertyTreeSerializer::serialize(this, -1, true, false);
        out << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented);
        _fileInfo = QFileInfo(file);
        file.close();
    }
    
    void Project::tileWindows()
    {
        // Get window frame borders.
        int frameLeft = 0;
        int frameRight = 0;
        int frameTop = 0;
        int frameBottom = 0;
        foreach(QWidget *widget, QApplication::topLevelWidgets()) {
            if(QMainWindow *window = qobject_cast<QMainWindow*>(widget)) {
                QRect frame = window->frameGeometry();
                QRect geo = window->geometry();
                frameLeft = geo.x() - frame.x();
                frameRight = frame.x() + frame.width() - (geo.x() + geo.width());
                frameTop = geo.y() - frame.y();
                frameBottom = frame.y() + frame.height() - (geo.y() + geo.height());
                break;
            }
        }
        // Get all windows.
        QList<QMainWindow*> windows;
        foreach(QWidget *widget, QApplication::topLevelWidgets()) {
            if(QMainWindow *window = qobject_cast<QMainWindow*>(widget))
                windows.push_back(window);
        }
        if(windows.isEmpty())
            return;
        QDesktopWidget *desktop = QApplication::desktop();
        // Sort windows by descending grid size.
        for(int i = 1; i < windows.size(); ++i) {
            int rowsi = 1;
            int colsi = 1;
            if(StimulusClampProtocol::StimulusClampProtocolWindow *protocolWindow = qobject_cast<StimulusClampProtocol::StimulusClampProtocolWindow*>(windows.at(i))) {
                rowsi = protocolWindow->plotRows();
                colsi = protocolWindow->plotColumns();
            }
            int sizei = rowsi * colsi;
            for(int j = i-1; j >= 0; --j) {
                int rowsj = 1;
                int colsj = 1;
                if(StimulusClampProtocol::StimulusClampProtocolWindow *protocolWindow = qobject_cast<StimulusClampProtocol::StimulusClampProtocolWindow*>(windows.at(j))) {
                    rowsj = protocolWindow->plotRows();
                    colsj = protocolWindow->plotColumns();
                }
                int sizej = rowsj * colsj;
                if(sizei > sizej || (sizei == sizej && rowsi > rowsj))
                    windows.swap(j, j + 1);
                else
                    break;
            }
        }
        // Split desktop into grid.
        int numUnitCells = 0;
        int minRows = 1;
        int minCols = 1;
        foreach(QMainWindow *window, windows) {
            int rows = 1;
            int cols = 1;
            if(StimulusClampProtocol::StimulusClampProtocolWindow *protocolWindow = qobject_cast<StimulusClampProtocol::StimulusClampProtocolWindow*>(window)) {
                rows = protocolWindow->plotRows();
                cols = protocolWindow->plotColumns();
            }
            if(rows > minRows)
                minRows = rows;
            if(cols > minCols)
                minCols = cols;
            numUnitCells += rows * cols;
        }
        int numRows = round(sqrt(numUnitCells));
        if(numRows < minRows)
            numRows = minRows;
        int numCols = ceil(float(numUnitCells) / numRows);
        if(numCols < minCols)
            numCols = minCols;
        QRect screen = desktop->availableGeometry();
        int unitCellWidth = floor(float(screen.width()) / numCols);
        int unitCellHeight = floor(float(screen.height()) / numRows);
        // Place windows into grid.
        QVector<QVector<int> > occupiedColRow;
        foreach(QMainWindow *window, windows) {
            int rows = 1;
            int cols = 1;
            if(StimulusClampProtocol::StimulusClampProtocolWindow *protocolWindow = qobject_cast<StimulusClampProtocol::StimulusClampProtocolWindow*>(window)) {
                rows = protocolWindow->plotRows();
                cols = protocolWindow->plotColumns();
            }
            // Try to put under previous window(s).
            bool windowPlaced = false;
            for(int firstCol = 0; firstCol + cols <= occupiedColRow.size(); ++firstCol) {
                for(int firstRow = 0; firstRow + rows <= numRows; ++firstRow) {
                    bool ok = true;
                    for(int col = firstCol; col < firstCol + cols; ++col) {
                        for(int row = firstRow; row < firstRow + rows; ++row) {
                            if(occupiedColRow[col].contains(row)) {
                                ok = false;
                                break;
                            }
                        }
                        if(!ok)
                            break;
                    }
                    if(ok) {
                        // Place under previous window(s).
                        int x = screen.x() + firstCol * unitCellWidth + frameLeft;
                        int y = screen.y() + firstRow * unitCellHeight + frameTop;
                        int w = cols * unitCellWidth - frameLeft - frameRight;
                        int h = rows * unitCellHeight - frameTop - frameBottom;
                        window->setGeometry(x, y, w, h);
                        windowPlaced = true;
                        for(int col = firstCol; col < firstCol + cols; ++col) {
                            for(int row = firstRow; row < firstRow + rows; ++row)
                                occupiedColRow[col].append(row);
                            qSort(occupiedColRow[col]);
                        }
                    }
                    if(windowPlaced)
                        break;
                }
                if(windowPlaced)
                    break;
            }
            // Put to right of previous windows in first row.
            if(!windowPlaced) {
                int firstRow = 0;
                int firstCol = occupiedColRow.size();
                int x = screen.x() + firstCol * unitCellWidth + frameLeft;
                int y = screen.y() + firstRow * unitCellHeight + frameTop;
                int w = cols * unitCellWidth - frameLeft - frameRight;
                int h = rows * unitCellHeight - frameTop - frameBottom;
                window->setGeometry(x, y, w, h);
                windowPlaced = true;
                for(int col = firstCol; col < firstCol + cols; ++col) {
                    if(col >= occupiedColRow.size())
                        occupiedColRow.resize(col+1);
                    for(int row = firstRow; row < firstRow + rows; ++row)
                        occupiedColRow[col].append(row);
                    qSort(occupiedColRow[col]);
                }
            }
        }
    }
    
    void Project::editOptions(const QString &title, const QList<QByteArray> &propertyNames)
    {
        QObjectPropertyEditor::QObjectPropertyModel propertyModel;
        propertyModel.setObject(this);
        
        if(!propertyNames.isEmpty())
            propertyModel.setPropertyNames(propertyNames);
        
        QObjectPropertyEditor::QObjectPropertyEditor *editor = new QObjectPropertyEditor::QObjectPropertyEditor();
        editor->setModel(&propertyModel);
        
        QDialog dialog;
        dialog.setWindowTitle(title);
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(editor);
        dialog.show();
        editor->resizeColumnToContents(0);
        if(editor->columnWidth(0) < 200)
            editor->setColumnWidth(0, 200);
        dialog.resize(500, 300);
        dialog.exec();
    }
    
    void Project::editSimulationOptions()
    {
        QList<QByteArray> propertyNames;
        propertyNames << "SimulationMethod" << "NumberOfMonteCarloRuns" << "AccumulateMonteCarloRuns" << "SampleMonteCarloProbability" << "NumberOfOptimizationIterations";
        editOptions("Simulation Options", propertyNames);
    }
    
    void Project::editWindowOptions()
    {
        QList<QByteArray> propertyNames;
        propertyNames << "AutoTileWindows";
        editOptions("Window Options", propertyNames);
    }
    
    void Project::simulate(MarkovModel::MarkovModel *model)
    {
        // Make sure we're not currently busy.
        if(_isBusy) return;
        _isBusy = true;
        _timer.start();
        
        // Model.
        _modelWindow = qobject_cast<MarkovModel::MarkovModelWindow*>(QApplication::activeWindow());
        if(!model && _modelWindow)
            model = _modelWindow->model();
        if(!model) {
            QErrorMessage errMsg;
            errMsg.showMessage("No model selected.");
            errMsg.exec();
            return;
        }
        
        // StimulusClampProtocols.
        StimulusClampProtocol::StimulusClampProtocolSimulator *stimulusClampProtocolSimulator = new StimulusClampProtocol::StimulusClampProtocolSimulator("Simulating " + model->name() + "...", _modelWindow);
        foreach(QWidget *widget, QApplication::topLevelWidgets()) {
            if(StimulusClampProtocol::StimulusClampProtocolWindow *window = qobject_cast<StimulusClampProtocol::StimulusClampProtocolWindow*>(widget)) {
                stimulusClampProtocolSimulator->protocols.push_back(window->protocol());
                connect(stimulusClampProtocolSimulator, SIGNAL(finished()), window, SLOT(replot()));
                connect(stimulusClampProtocolSimulator, SIGNAL(finished()), window, SLOT(showMaxProbabilityError()));
            }
        }
        if(stimulusClampProtocolSimulator->protocols.size()) {
            stimulusClampProtocolSimulator->model = model;
            if(_simulationMethod == EigenSolver) {
                stimulusClampProtocolSimulator->options["Method"] = "Eigen Solver";
            } else if(_simulationMethod == MonteCarlo) {
                stimulusClampProtocolSimulator->options["Method"] = "Monte Carlo";
                stimulusClampProtocolSimulator->options["# Monte Carlo runs"] = _numMonteCarloRuns;
                stimulusClampProtocolSimulator->options["Accumulate Monte Carlo runs"] = _accumulateMonteCarloRuns;
                stimulusClampProtocolSimulator->options["Sample probability from Monte Carlo event chains"] = _sampleProbabilityFromMonteCarloEventChains;
            }
            connect(stimulusClampProtocolSimulator, SIGNAL(finished()), this, SLOT(simulationFinished()));
            stimulusClampProtocolSimulator->simulate();
        } else {
            delete stimulusClampProtocolSimulator;
            _isBusy = false;
        }
    }
    
    void Project::optimize(MarkovModel::MarkovModel *model)
    {
        // Make sure we're not currently busy.
        if(_isBusy) return;
        _isBusy = true;
        _timer.start();
        
        // Model.
        _modelWindow = qobject_cast<MarkovModel::MarkovModelWindow*>(QApplication::activeWindow());
        if(!model && _modelWindow)
            model = _modelWindow->model();
        if(!model) {
            QErrorMessage errMsg;
            errMsg.showMessage("No model selected.");
            errMsg.exec();
            return;
        }
        
        // StimulusClampProtocols.
        StimulusClampProtocol::StimulusClampProtocolSimulator *stimulusClampProtocolSimulator = new StimulusClampProtocol::StimulusClampProtocolSimulator("Simulating " + model->name() + "...", _modelWindow);
        foreach(QWidget *widget, QApplication::topLevelWidgets()) {
            if(StimulusClampProtocol::StimulusClampProtocolWindow *window = qobject_cast<StimulusClampProtocol::StimulusClampProtocolWindow*>(widget)) {
                stimulusClampProtocolSimulator->protocols.push_back(window->protocol());
                connect(stimulusClampProtocolSimulator, SIGNAL(finished()), window, SLOT(replot()));
                connect(stimulusClampProtocolSimulator, SIGNAL(finished()), window, SLOT(showMaxProbabilityError()));
            }
        }
        if(stimulusClampProtocolSimulator->protocols.size()) {
            stimulusClampProtocolSimulator->model = model;
            if(_simulationMethod == EigenSolver) {
                stimulusClampProtocolSimulator->options["Method"] = "Eigen Solver";
            } else if(_simulationMethod == MonteCarlo) {
                stimulusClampProtocolSimulator->options["Method"] = "Monte Carlo";
                stimulusClampProtocolSimulator->options["# Monte Carlo runs"] = _numMonteCarloRuns;
                stimulusClampProtocolSimulator->options["Accumulate Monte Carlo runs"] = _accumulateMonteCarloRuns;
                stimulusClampProtocolSimulator->options["Sample probability from Monte Carlo event chains"] = _sampleProbabilityFromMonteCarloEventChains;
            }
            connect(stimulusClampProtocolSimulator, SIGNAL(finished()), this, SLOT(simulationFinished()));
            stimulusClampProtocolSimulator->optimize(_numOptimizationIterations); // Will delete itself when simulation is done.
        } else {
            delete stimulusClampProtocolSimulator;
            _isBusy = false;
        }
    }
    
    void Project::simulationFinished()
    {
        if(_modelWindow) {
            _modelWindow->repaint();
            _modelWindow->statusBar()->showMessage("Elapsed time: " + QString::number(_timer.elapsed() / 1000.0) + " sec");
        }
        _modelWindow = 0;
        _isBusy = false;
    }

} // KineticModelBuilder
