/* --------------------------------------------------------------------------------
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __Project_H__
#define __Project_H__

#include "QObjectPropertyTreeSerializer.h"
#include <QFileInfo>
#include <QObject>
#include <QString>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif
class QMenu;
namespace MarkovModel { class MarkovModel; }
namespace StimulusClampProtocol { class StimulusClampProtocol; }

namespace KineticModelBuilder
{
    // For dynamic object creation.
    QObjectPropertyTreeSerializer::ObjectFactory getObjectFactory();
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class Project : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Version READ version)
        Q_PROPERTY(SimulationMethod SimulationMethod READ simulationMethod WRITE setSimulationMethod)
        Q_PROPERTY(int NumberOfMonteCarloRuns READ numMonteCarloRuns WRITE setNumMonteCarloRuns)
        Q_PROPERTY(bool AccumulateMonteCarloRuns READ accumulateMonteCarloRuns WRITE setAccumulateMonteCarloRuns)
        Q_PROPERTY(bool SampleMonteCarloProbability READ sampleProbabilityFromMonteCarloEventChains WRITE setSampleProbabilityFromMonteCarloEventChains)
        Q_PROPERTY(int NumberOfOptimizationIterations READ numOptimizationIterations WRITE setNumOptimizationIterations)
        Q_PROPERTY(bool AutoTileWindows READ autoTileWindows WRITE setAutoTileWindows)
        
    public:
        enum  SimulationMethod { EigenSolver, MonteCarlo };
        Q_ENUMS(SimulationMethod)
        
        // For dynamic object creation.
        static QObjectPropertyTreeSerializer::ObjectFactory objectFactory;
        
        Project() : _simulationMethod(MonteCarlo), _numMonteCarloRuns(1000), _accumulateMonteCarloRuns(false), _sampleProbabilityFromMonteCarloEventChains(true), _numOptimizationIterations(100), _autoTileWindows(true), _busyLevel(0) {}
        
        // Property getters.
        static QString version() { return "4.2.0"; }
        SimulationMethod simulationMethod() const { return _simulationMethod; }
        int numMonteCarloRuns() const { return _numMonteCarloRuns; }
        bool accumulateMonteCarloRuns() const { return _accumulateMonteCarloRuns; }
        bool sampleProbabilityFromMonteCarloEventChains() const { return _sampleProbabilityFromMonteCarloEventChains; }
        int numOptimizationIterations() const { return _numOptimizationIterations; }
        bool autoTileWindows() const { return _autoTileWindows; }
        
        // Property setters.
        void setSimulationMethod(SimulationMethod s) { _simulationMethod = s; }
        void setNumMonteCarloRuns(int n) { _numMonteCarloRuns = n; }
        void setAccumulateMonteCarloRuns(bool b) { _accumulateMonteCarloRuns = b; }
        void setSampleProbabilityFromMonteCarloEventChains(bool b) { _sampleProbabilityFromMonteCarloEventChains = b; }
        void setNumOptimizationIterations(int n) { _numOptimizationIterations = n; }
        void setAutoTileWindows(bool b) { _autoTileWindows = b; }
        
        QMenu* newMenu();
        
#ifdef DEBUG
        void dump(std::ostream &out = std::cout);
#endif
        
    public slots:
        void about();
        void newMarkovModel();
        void newStimulusClampProtocol();
        void open(QString filePath = "");
        void saveAs(QString filePath = "");
        void tileWindows();
        void updateWindows() { if(_autoTileWindows) tileWindows(); }
        void editOptions(const QString &title = "Options", const QList<QByteArray> &propertyNames = QList<QByteArray>());
        void editSimulationOptions();
        void editWindowOptions();
        void simulate(MarkovModel::MarkovModel *model = 0);
        void optimize(MarkovModel::MarkovModel *model = 0);
        void simulationFinished();
        
    protected:
        // Properties.
        SimulationMethod _simulationMethod;
        int _numMonteCarloRuns;
        bool _accumulateMonteCarloRuns;
        bool _sampleProbabilityFromMonteCarloEventChains;
        int _numOptimizationIterations;
        bool _autoTileWindows;
        
        // File info.
        QFileInfo _fileInfo;
        
        // Busy indicator.
        int _busyLevel;
        
        // Creates a new Object and a UI for it.
        template <class Object, class UI>
        Object* _newObjectWithUI(const QVariantMap &data = QVariantMap())
        {
            Object *object = new Object(this);
            if(!data.isEmpty()) {
                object->clear();
                QObjectPropertyTreeSerializer::deserialize(object, data, &Object::objectFactory);
            }
            UI *ui = new UI(object);
            ui->setAttribute(Qt::WA_DeleteOnClose, true);
            connect(ui, SIGNAL(destroyed()), object, SLOT(deleteLater()));
            connect(ui, SIGNAL(destroyed()), this, SLOT(updateWindows()));
            ui->show();
            return object;
        }
    };
    
} // KineticModelBuilder

#endif
