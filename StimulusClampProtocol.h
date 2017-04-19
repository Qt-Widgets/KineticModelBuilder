/* --------------------------------------------------------------------------------
 *
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __StimulusClampProtocol_H__
#define __StimulusClampProtocol_H__

#include <array>
#include <atomic>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <QCloseEvent>
#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QString>
#include <QWidget>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include "MarkovModel.h"
#include "QObjectPropertyTreeSerializer.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace StimulusClampProtocol
{
    typedef std::atomic<bool> AbortFlag;
    
    /* --------------------------------------------------------------------------------
     * Get a pre-seeded random number generator.
     * -------------------------------------------------------------------------------- */
    template <typename T = std::mt19937>
    T getSeededRandomNumberGenerator()
    {
        std::random_device rd;
        std::array<std::random_device::result_type, T::state_size> seed_data;
        std::generate_n(seed_data.data(), seed_data.size(), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        T rng(seq);
        return rng;
    }
    
    /* --------------------------------------------------------------------------------
     * Info for a Monte Carlo event.
     * -------------------------------------------------------------------------------- */
    struct MonteCarloEvent
    {
        int state; // state index
        double duration; // time duration
        
        MonteCarloEvent(int state = -1, double duration = 0) : state(state), duration(duration) {}
    };
    typedef std::vector<MonteCarloEvent> MonteCarloEventChain;
    
    /* --------------------------------------------------------------------------------
     * Equilibrium state probabilities from transition rates Q matrix.
     * !!! Note that if you have the spectral expansion, then the equilibrium state probabilities
     *     are just the initial state probabilities multiplied by the spectral matrix with zero eigenvalue.
     * -------------------------------------------------------------------------------- */
    Eigen::RowVectorXd equilibriumProbability(const Eigen::MatrixXd &Q);
    
    /* --------------------------------------------------------------------------------
     * Spectral expansion of unitary transition rates Q matrix.
     * !!! This will invalidate the input matrix Q.
     * -------------------------------------------------------------------------------- */
    void spectralExpansion(const Eigen::SparseMatrix<double> &Q, Eigen::VectorXd &eigenValues, std::vector<Eigen::MatrixXd> &spectralMatrices, AbortFlag *abort = 0);
    
    /* --------------------------------------------------------------------------------
     * Find sample indexes in range.
     * -------------------------------------------------------------------------------- */
    void findIndexesInRange(const Eigen::VectorXd &time, double start, double stop, int *firstPt, int *numPts);
    
    /* --------------------------------------------------------------------------------
     * For dynamic object creation.
     * -------------------------------------------------------------------------------- */
    QObjectPropertyTreeSerializer::ObjectFactory getObjectFactory();
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class Stimulus : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(bool Active READ isActive WRITE setIsActive)
        Q_PROPERTY(QString Start READ start WRITE setStart)
        Q_PROPERTY(QString Duration READ duration WRITE setDuration)
        Q_PROPERTY(QString Amplitude READ amplitude WRITE setAmplitude)
        Q_PROPERTY(QString OnsetExpr READ onsetExpr WRITE setOnsetExpr)
        Q_PROPERTY(QString OffsetExpr READ offsetExpr WRITE setOffsetExpr)
        Q_PROPERTY(QString Repetitions READ repetitions WRITE setRepetitions)
        Q_PROPERTY(QString Period READ period WRITE setPeriod)
        
    public:
        // Default constructor.
        Stimulus(QObject *parent = 0, const QString &name = "") :
        QObject(parent), _isActive(true), _repetitions("1"), _period("0") { setName(name); }
        
        // Property getters.
        QString name() const { return objectName(); }
        bool isActive() const { return _isActive; }
        QString start() const { return _start; }
        QString duration() const { return _duration; }
        QString amplitude() const { return _amplitude; }
        QString onsetExpr() const { return _onsetExpr; }
        QString offsetExpr() const { return _offsetExpr; }
        QString repetitions() const { return _repetitions; }
        QString period() const { return _period; }
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setIsActive(bool b) { _isActive = b; }
        void setStart(QString s) { _start = s; }
        void setDuration(QString s) { _duration = s; }
        void setAmplitude(QString s) { _amplitude = s; }
        void setOnsetExpr(QString s) { _onsetExpr = s; }
        void setOffsetExpr(QString s) { _offsetExpr = s; }
        void setRepetitions(QString s) { _repetitions = s; }
        void setPeriod(QString s) { _period = s; }
        
        // Conditions matrices.
        std::vector<std::vector<double> > starts;
        std::vector<std::vector<double> > durations;
        std::vector<std::vector<double> > amplitudes;
        std::vector<std::vector<std::string> > onsetExprs;
        std::vector<std::vector<std::string> > offsetExprs;
        std::vector<std::vector<int> > repeats;
        std::vector<std::vector<double> > periods;
        
        Eigen::VectorXd waveform(Eigen::VectorXd &time, int row, int col);
        
    protected:
        // Properties.
        bool _isActive;
        QString _start;
        QString _duration;
        QString _amplitude;
        QString _onsetExpr;
        QString _offsetExpr;
        QString _repetitions;
        QString _period;
    };
    
    /* --------------------------------------------------------------------------------
     * Period of constant stimuli.
     * -------------------------------------------------------------------------------- */
    struct Epoch
    {
        // Defines the epoch.
        std::map<QString, double> stimuli;
        
        // Time period and sample indexes.
        double start;
        double duration;
        int firstPt;
        int numPts;
        
        // Ref to a unique epoch that will have all the data associated with this epoch.
        Epoch *uniqueEpoch;
        
        // Data to be computed for unique epochs only.
        Eigen::RowVectorXd stateProbabilities;
        std::map<QString, Eigen::RowVectorXd> stateAttributes;
        Eigen::SparseMatrix<double> transitionRates; // _ij = rate i->j, _ii = negative sum of rates leaving state i.
        Eigen::SparseMatrix<double> transitionCharges; // _ij = charge moved during transition i->j.
        Eigen::RowVectorXd stateChargeCurrents;
        
        // Spectral expansion of transitionRates matrix.
        Eigen::VectorXd spectralEigenValues;
        std::vector<Eigen::MatrixXd> spectralMatrices;
        
        // For exponentially distributed random state lifetimes.
        std::vector<std::exponential_distribution<double> > randomStateLifetimes;
        
        Epoch(double start = 0) : start(start), duration(0), firstPt(-1), numPts(0), uniqueEpoch(0) {}
        
        // For sorting epochs based on start time.
        bool operator < (const Epoch &epoch) const { return start < epoch.start; }
    };
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    struct Simulation
    {
        Eigen::VectorXd time;
        double endTime;
        std::map<QString, Eigen::VectorXd> stimuli;
        std::vector<Epoch> epochs;
        Eigen::VectorXd weight;
        Eigen::Matrix<bool, Eigen::Dynamic, 1> mask;
        
        // List of simulations for each variable set.
        std::vector<Eigen::MatrixXd> probability; // Columns are time-dependent probability in each state.
        std::vector<std::map<QString, Eigen::VectorXd> > waveforms;
        std::vector<std::vector<MonteCarloEventChain> > events;
        
        // Random number generator.
        std::mt19937 randomNumberGenerator;
        
        void findEpochsDiscretizedToSamplePoints();
        void spectralSimulation(Eigen::RowVectorXd startingProbability, bool startEquilibrated = false, size_t variableSetIndex = 0, AbortFlag *abort = 0, QString *message = 0);
        void monteCarloSimulation(Eigen::RowVectorXd startingProbability, std::mt19937 &randomNumberGenerator, size_t numRuns, bool accumulateRuns = false, bool sampleRuns = true, bool startEquilibrated = false, size_t variableSetIndex = 0, AbortFlag *abort = 0, QString *message = 0);
        void getProbabilityFromEventChains(Eigen::MatrixXd &P, size_t numStates, const std::vector<MonteCarloEventChain> &eventChains, AbortFlag *abort = 0, QString *message = 0);
        double maxProbabilityError();
    };
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class Waveform : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(bool Active READ isActive WRITE setIsActive)
        Q_PROPERTY(QString Expr READ expr WRITE setExpr)
        
    public:
        // Default constructor.
        Waveform(QObject *parent = 0, const QString &name = "") :
        QObject(parent), _isActive(true) { setName(name); }
        
        // Property getters.
        QString name() const { return objectName(); }
        bool isActive() const { return _isActive; }
        QString expr() const { return _expr; }
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setIsActive(bool b) { _isActive = b; }
        void setExpr(QString s) { _expr = s; }
        
    protected:
        // Properties.
        bool _isActive;
        QString _expr;
    };
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class SimulationsSummary : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(bool Active READ isActive WRITE setIsActive)
        Q_PROPERTY(QString ExprX READ exprX WRITE setExprX)
        Q_PROPERTY(QString ExprY READ exprY WRITE setExprY)
        Q_PROPERTY(QString StartX READ startX WRITE setStartX)
        Q_PROPERTY(QString DurationX READ durationX WRITE setDurationX)
        Q_PROPERTY(QString StartY READ startY WRITE setStartY)
        Q_PROPERTY(QString DurationY READ durationY WRITE setDurationY)
        Q_PROPERTY(Normalization Normalization READ normalization WRITE setNormalization)
        
    public:
        enum Normalization { None, PerRow, AllRows };
        Q_ENUMS(Normalization);
        
        typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMajorMatrixXd;
        typedef Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMajorMatrixXi;
        
        // Default constructor.
        SimulationsSummary(QObject *parent = 0, const QString &name = "") :
        QObject(parent), _isActive(true), _normalization(None) { setName(name); }
        
        // Property getters.
        QString name() const { return objectName(); }
        bool isActive() const { return _isActive; }
        QString exprX() const { return _exprX; }
        QString exprY() const { return _exprY; }
        QString startX() const { return _startX; }
        QString durationX() const { return _durationX; }
        QString startY() const { return _startY; }
        QString durationY() const { return _durationY; }
        Normalization normalization() const { return _normalization; }
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setIsActive(bool b) { _isActive = b; }
        void setExprX(QString s) { _exprX = s; }
        void setExprY(QString s) { _exprY = s; }
        void setStartX(QString s) { _startX = s; }
        void setDurationX(QString s) { _durationX = s; }
        void setStartY(QString s) { _startY = s; }
        void setDurationY(QString s) { _durationY = s; }
        void setNormalization(Normalization i) { _normalization = i; }
        
        // Conditions matrices.
        std::vector<std::vector<std::string> > exprXs;
        std::vector<std::vector<std::string> > exprYs;
        std::vector<std::vector<double> > startXs;
        std::vector<std::vector<double> > durationXs;
        std::vector<std::vector<double> > startYs;
        std::vector<std::vector<double> > durationYs;
        
        // Indexes.
        RowMajorMatrixXi firstPtX;
        RowMajorMatrixXi numPtsX;
        RowMajorMatrixXi firstPtY;
        RowMajorMatrixXi numPtsY;
        
        // Summary.
        std::vector<RowMajorMatrixXd> dataX;
        std::vector<RowMajorMatrixXd> dataY;
        
    protected:
        // Properties.
        bool _isActive;
        QString _exprX;
        QString _exprY;
        QString _startX;
        QString _durationX;
        QString _startY;
        QString _durationY;
        Normalization _normalization;
    };
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class StimulusClampProtocol : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(QString Notes READ notes WRITE setNotes)
        Q_PROPERTY(QString Start READ start WRITE setStart)
        Q_PROPERTY(QString Duration READ duration WRITE setDuration)
        Q_PROPERTY(QString SampleInterval READ sampleInterval WRITE setSampleInterval)
        Q_PROPERTY(QString Weight READ weight WRITE setWeight)
        Q_PROPERTY(bool StartEquilibrated READ startEquilibrated WRITE setStartEquilibrated)
        
    public:
        // For dynamic object creation.
        static QObjectPropertyTreeSerializer::ObjectFactory objectFactory;
        
        // Matrix of simulations (one for each set of conditions).
        std::vector<std::vector<Simulation> > simulations;
        QStringList stateNames;
        
        // Default constructor.
        StimulusClampProtocol(QObject *parent = 0, const QString &name = "");
        
        // Property getters.
        QString name() const { return objectName(); }
        QString notes() const { return _notes; }
        QString start() const { return _start; }
        QString duration() const { return _duration; }
        QString sampleInterval() const { return _sampleInterval; }
        QString weight() const { return _weight; }
        bool startEquilibrated() const { return _startEquilibrated; }
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setNotes(QString s) { _notes = s; }
        void setStart(QString s) { _start = s; }
        void setDuration(QString s) { _duration = s; }
        void setSampleInterval(QString s) { _sampleInterval = s; }
        void setWeight(QString s) { _weight = s; }
        void setStartEquilibrated(bool b) { _startEquilibrated = b; }
        
        // Conditions matrices.
        std::vector<std::vector<double> > starts;
        std::vector<std::vector<double> > durations;
        std::vector<std::vector<double> > sampleIntervals;
        std::vector<std::vector<double> > weights;
        
        // Initialize prior to running a simulation.
        void init(std::vector<Epoch*> &uniqueEpochs);
        
        // Cost function.
        double cost();
        
#ifdef DEBUG
        void dump(std::ostream &out = std::cout);
#endif
        
    public slots:
        void clear();
        void open(QString filePath = "");
        void save();
        void saveAs(QString filePath = "");
        void saveMonteCarloEventChainsAsDwt(QString filePath = "");
        
    protected:
        // Properties.
        QString _notes;
        QString _start;
        QString _duration;
        QString _sampleInterval;
        QString _weight;
        bool _startEquilibrated;
        
        // File info.
        QFileInfo _fileInfo;
    };
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class StimulusClampProtocolSimulator
    {
    public:
        MarkovModel::MarkovModel *model;
        std::vector<StimulusClampProtocol*> protocols;
        QVariantMap options;
        
        QStringList stateNames;
        std::vector<Epoch*> uniqueEpochs;
        AbortFlag abort;
        QString message;
        
        StimulusClampProtocolSimulator() : model(0), abort(false) {}
        ~StimulusClampProtocolSimulator() { for(Epoch *epoch : uniqueEpochs) delete epoch; }
        
        void init();
        void run();
    };
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class StimulusClampProtocolSimulatorDialog : public QProgressDialog, public StimulusClampProtocolSimulator
    {
        Q_OBJECT
        
    public:
        StimulusClampProtocolSimulatorDialog(const QString &labelText = "", QWidget *parent = 0);
        
        void simulate();
        
    signals:
        void aborted();
        void finished();
        
    protected slots:
        void _abort();
        void _finish();
        
    protected:
        QFuture<void> _future;
        QFutureWatcher<void> _watcher;
        
        void closeEvent(QCloseEvent *event) { _abort(); event->accept(); }
    };
    
    /* --------------------------------------------------------------------------------
     * Parse string rep of value array.
     * Numeric value ranges may be specified as start:step:stop.
     * !!! Currently all value conversion failures are ignored.
     * -------------------------------------------------------------------------------- */
    template <typename T>
    std::vector<T> str2vec(const QString &str, const QString &delimiterRegex = "[,\\s]\\s*", const QString &rangeDelimiterRegex = ":")
    {
        std::vector<T> vec;
        std::istringstream iss;
        T value;
        QStringList fields = str.split(QRegularExpression(delimiterRegex), QString::SkipEmptyParts);
        vec.reserve(fields.size());
        foreach(QString field, fields) {
            field = field.trimmed();
            if(!field.isEmpty()) {
                QStringList subfields = field.split(QRegularExpression(rangeDelimiterRegex), QString::SkipEmptyParts);
                if(subfields.size() == 1) { // single value
                    iss.clear();
                    iss.str(field.toStdString());
                    iss >> value;
                    if(!iss.fail() && !iss.bad() && iss.eof())
                        vec.push_back(value);
                } else if(subfields.size() == 2) { // start:stop
                    T start;
                    iss.clear();
                    iss.str(subfields[0].toStdString());
                    iss >> start;
                    if(!iss.fail() && !iss.bad() && iss.eof()) {
                        T stop;
                        iss.clear();
                        iss.str(subfields[1].toStdString());
                        iss >> stop;
                        if(!iss.fail() && !iss.bad() && iss.eof()) {
                            for(value = start; value <= stop; value += 1)
                                vec.push_back(value);
                        }
                    }
                } else if(subfields.size() == 3) { // start:step:stop
                    T start;
                    iss.clear();
                    iss.str(subfields[0].toStdString());
                    iss >> start;
                    if(!iss.fail() && !iss.bad() && iss.eof()) {
                        T step;
                        iss.clear();
                        iss.str(subfields[1].toStdString());
                        iss >> step;
                        if(!iss.fail() && !iss.bad() && iss.eof()) {
                            T stop;
                            iss.clear();
                            iss.str(subfields[2].toStdString());
                            iss >> stop;
                            if(!iss.fail() && !iss.bad() && iss.eof()) {
                                if(step > 0) {
                                    for(value = start; value <= stop; value += step)
                                        vec.push_back(value);
                                } else if(step < 0) {
                                    for(value = start; value >= stop; value += step)
                                        vec.push_back(value);
                                }
                            }
                        }
                    }
                }
            }
        }
        return vec;
    }
    // Specialization for strings because ranges don't make sense for strings.
    template <>
    std::vector<std::string> str2vec<std::string>(const QString &str, const QString &delimiterRegex, const QString &rangeDelimiterRegex);
    
    /* --------------------------------------------------------------------------------
     * Parse string rep of 2D matrix.
     * -------------------------------------------------------------------------------- */
    template <typename T>
    std::vector<std::vector<T> > str2mat(const QString &str, const QString &rowDelimiterRegex = ";", const QString &columnDelimiterRegex = "[,\\s]\\s*", const QString &rangeDelimiterRegex = ":")
    {
        std::vector<std::vector<T> > mat;
        QStringList rows = str.split(QRegularExpression(rowDelimiterRegex), QString::SkipEmptyParts);
        mat.reserve(rows.size());
        foreach(QString row, rows) {
            row = row.trimmed();
            if(!row.isEmpty()) {
                std::vector<T> vec = str2vec<T>(row, columnDelimiterRegex, rangeDelimiterRegex);
                if(!vec.empty())
                    mat.push_back(vec);
            }
        }
        return mat;
    }
    
    /* --------------------------------------------------------------------------------
     * Pad matrix out to rows x cols.
     * -------------------------------------------------------------------------------- */
    template <typename T>
    void padmat(std::vector<std::vector<T> > &mat, size_t rows, size_t cols, T defaultValue)
    {
        // Add/Remove columns to/from each row as necessary.
        for(size_t row = 0; row < mat.size(); ++row) {
            if(mat[row].empty())
                mat[row].push_back(defaultValue);
            for(size_t col = mat[row].size(); col < cols; ++col)
                mat[row].push_back(mat[row].back());
            while(mat[row].size() > cols)
                mat[row].pop_back();
        }
        // Add/Remove rows as necessary.
        if(mat.empty())
            mat.push_back(std::vector<T>(cols, defaultValue));
        for(size_t row = mat.size(); row < rows; ++row)
            mat.push_back(mat.back());
        while(mat.size() > rows)
            mat.pop_back();
    }
    
    /* --------------------------------------------------------------------------------
     * Get max number or rows x cols specified in input mat.
     * -------------------------------------------------------------------------------- */
    template <typename T>
    void matlims(std::vector<std::vector<T> > &mat, size_t *maxRows, size_t *maxCols)
    {
        if(mat.size() > *maxRows)
            *maxRows = mat.size();
        for(size_t row = 0; row < mat.size(); ++row) {
            if(mat[row].size() > *maxCols)
                *maxCols = mat[row].size();
        }
    }
    
#ifdef DEBUG
    void test();
#endif
    
} // StimulusClampProtocol

#endif
