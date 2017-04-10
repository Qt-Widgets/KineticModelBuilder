/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "StimulusClampProtocol.h"
#include "QObjectPropertyEditor.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <QFuture>
#include <QJsonDocument>
#include <QVariantMap>
#include <QtConcurrentRun>

namespace StimulusClampProtocol
{
    Eigen::RowVectorXd equilibriumProbability(const Eigen::MatrixXd &Q)
    {
        int N = Q.cols(); // # of states.
        // S is a copy of Q with one additional column of ones.
        Eigen::MatrixXd S = Eigen::MatrixXd::Ones(N, N + 1);
        S.block(0, 0, N, N) = Q;
        // u is a row vector of ones.
        Eigen::RowVectorXd u = Eigen::RowVectorXd::Ones(N);
        // Return u * ( S * S^T )^-1
        Eigen::MatrixXd St = S.transpose();
        return u * ((S * St).inverse());
    }
    
    void spectralExpansion(const Eigen::SparseMatrix<double> &Q, Eigen::VectorXd &eigenValues, std::vector<Eigen::MatrixXd> &spectralMatrices, AbortFlag *abort)
    {
        int N = Q.cols(); // # of states.
        if(N < 2) throw std::runtime_error("Spectral expansion for less than two states does not make sense.");
        Eigen::EigenSolver<Eigen::MatrixXd> eigenSolver(Q.toDense(), true);
        if(abort && *abort) return;
        Eigen::VectorXd eigVals = eigenSolver.pseudoEigenvalueMatrix().diagonal();
        // Get indexes of eigVals sorted in ascending order of their absolute value.
        std::vector<size_t> indexes(eigVals.size());
        std::iota(indexes.begin(), indexes.end(), 0); // indexes = {0, 1, 2, ...}
        // Sort indexes based on comparing values in eigenvalues.
        std::sort(indexes.begin(), indexes.end(), [&eigVals](size_t i1, size_t i2) { return fabs(eigVals[i1]) < fabs(eigVals[i2]); });
        if(abort && *abort) return;
        Eigen::MatrixXd invEigVecs = eigenSolver.pseudoEigenvectors().inverse();
        eigenValues = Eigen::VectorXd::Zero(N);
        spectralMatrices.assign(N, Eigen::MatrixXd::Zero(N, N));
        for(int i = 0; i < N; ++i) {
            if(abort && *abort) return;
            int j = indexes[i];
            eigenValues[i] = eigVals[j];
            spectralMatrices[i] = eigenSolver.pseudoEigenvectors().col(j) * invEigVecs.row(j);
        }
    }
    
    QObjectPropertyTreeSerializer::ObjectFactory getObjectFactory()
    {
        QObjectPropertyTreeSerializer::ObjectFactory factory;
        factory.registerCreator("Stimulus", factory.defaultCreator<Stimulus>);
        factory.registerCreator("StimulusClampProtocol", factory.defaultCreator<StimulusClampProtocol>);
        return factory;
    }
    QObjectPropertyTreeSerializer::ObjectFactory StimulusClampProtocol::objectFactory = getObjectFactory();
    
    Eigen::VectorXd Stimulus::waveform(Eigen::VectorXd &time, int row, int col)
    {
        int numPts = time.size();
        Eigen::VectorXd stimulusWaveform = Eigen::VectorXd::Zero(numPts);
//        double dt = (time.segment(1, numPts - 1) - time.segment(0, numPts - 1)).minCoeff() * 1e-5;
        double epsilon = std::numeric_limits<double>::epsilon() * 5;
        EigenLab::ParserXd parser;
        if(durations[row][col] > epsilon && fabs(amplitudes[row][col]) > epsilon) {
            for(int rep = 0; rep < repeats[row][col]; ++rep) {
                double onsetTime = starts[row][col] + rep * periods[row][col];
                double offsetTime = onsetTime + durations[row][col];
                Eigen::VectorXd::Index closestIndex;
                (time.array() - onsetTime).abs().minCoeff(&closestIndex);
                int firstOnsetPt = closestIndex;
                if(time[firstOnsetPt] < onsetTime - epsilon)
                    ++firstOnsetPt;
                if(firstOnsetPt < time.size()) {
                    (time.array() - offsetTime).abs().minCoeff(&closestIndex);
                    int firstOffsetPt = closestIndex;
                    if(time[firstOffsetPt] < offsetTime - epsilon)
                        ++firstOffsetPt;
                    int numOnsetPts = firstOffsetPt - firstOnsetPt;
                    int numOffsetPts = time.size() - firstOffsetPt;
                    if(onsetExprs[row][col].size() || offsetExprs[row][col].size()) {
                        if(numOnsetPts > 0 && onsetExprs[row][col].size()) {
                            try {
                                Eigen::VectorXd pulseTime = time.segment(firstOnsetPt, numOnsetPts).array() - onsetTime;
                                parser.var("t").setShared(pulseTime.data(), pulseTime.size(), 1);
                                stimulusWaveform.segment(firstOnsetPt, numOnsetPts)
                                += (parser.eval(onsetExprs[row][col]).matrix().array() * amplitudes[row][col]).matrix();
                            } catch(...) {
                            }
                        }
                        if(numOffsetPts > 0 && offsetExprs[row][col].size()) {
                            try {
                                Eigen::VectorXd pulseTime = time.segment(firstOffsetPt, numOffsetPts).array() - offsetTime;
                                parser.var("t").setShared(pulseTime.data(), pulseTime.size(), 1);
                                stimulusWaveform.segment(firstOffsetPt, numOffsetPts)
                                += (parser.eval(offsetExprs[row][col]).matrix().array() * amplitudes[row][col]).matrix();
                            } catch(...) {
                            }
                        }
                    } else {
                        // Square pulse.
                        if(numOnsetPts > 0)
                            stimulusWaveform.segment(firstOnsetPt, numOnsetPts) += Eigen::VectorXd::Constant(numOnsetPts, amplitudes[row][col]);
                    }
                }
            }
        }
        return stimulusWaveform;
    }
    
    void Simulation::findEpochsDiscretizedToSamplePoints()
    {
        epochs.clear();
        Epoch epoch;
        epoch.start = time[0];
        epoch.firstPt = 0;
        for(auto &kv : stimuli)
            epoch.stimuli[kv.first] = kv.second[0];
        epochs.push_back(epoch);
        int numPts = time.size();
        for(int i = 1; i < numPts; ++i) {
            for(auto &kv : stimuli) {
                if(kv.second[i] != kv.second[i - 1]) {
                    epochs.back().duration = time[i] - epochs.back().start;
                    epochs.back().numPts = i - epochs.back().firstPt;
                    epoch.start = time[i];
                    epoch.firstPt = i;
                    for(auto &kv2 : stimuli)
                        epoch.stimuli[kv2.first] = kv2.second[i];
                    epochs.push_back(epoch);
                    break;
                }
            }
        }
        epochs.back().duration = endTime - epochs.back().start;
        epochs.back().numPts = numPts - epochs.back().firstPt;
    }
    
    void Simulation::spectralSimulation(Eigen::RowVectorXd startingProbability, bool startEquilibrated, size_t variableSetIndex, AbortFlag *abort, QString */* message */)
    {
        int numPts = time.size();
        int numStates = startingProbability.size();
        while(probability.size() < variableSetIndex)
            probability.push_back(Eigen::MatrixXd::Zero(numPts, numStates));
        Eigen::MatrixXd &P = probability.at(variableSetIndex);
        P.setZero(numPts, numStates);
        size_t epochCounter = 0;
        for(const Epoch &epoch : epochs) {
            if(abort && *abort) return;
            if(epochCounter == 0 && startEquilibrated) {
                // Set first epoch to equilibrium probabilities.
                startingProbability = startingProbability * epoch.uniqueEpoch->spectralMatrices[0];
                if(epoch.numPts > 0)
                    P.block(epoch.firstPt, 0, epoch.numPts, numStates).rowwise() = startingProbability;
            } else {
                if(epoch.numPts > 0) {
                    // Compute epoch probability using Q matrix spectral expansion.
                    Eigen::VectorXd epochTime = time.segment(epoch.firstPt, epoch.numPts).array() - epoch.start;
                    for(int i = 0; i < numStates; ++i) {
                        if(abort && *abort) return;
                        double lambda = epoch.uniqueEpoch->spectralEigenValues[i];
                        Eigen::MatrixXd &A = epoch.uniqueEpoch->spectralMatrices[i];
                        P.block(epoch.firstPt, 0, epoch.numPts, numStates) += (epochTime.array() * lambda).exp().matrix() * (startingProbability * A);
                    }
                }
                if(epochCounter + 1 < epochs.size()) {
                    // Update starting probability for next epoch.
                    Eigen::RowVectorXd temp = Eigen::RowVectorXd::Zero(numStates);
                    for(int i = 0; i < numStates; ++i) {
                        if(abort && *abort) return;
                        double lambda = epoch.uniqueEpoch->spectralEigenValues[i];
                        Eigen::MatrixXd &A = epoch.uniqueEpoch->spectralMatrices[i];
                        temp += ((startingProbability * A).array() * exp(lambda * epoch.duration)).matrix();
                    }
                    startingProbability = temp;
                }
            }
            ++epochCounter;
        }
    }
    
    void Simulation::monteCarloSimulation(Eigen::RowVectorXd startingProbability, std::mt19937 &randomNumberGenerator, size_t numRuns, bool accumulateRuns, bool startEquilibrated, size_t variableSetIndex, AbortFlag *abort, QString */* message */)
    {
        int numStates = startingProbability.size();
        if(events.size() < variableSetIndex)
            events.resize(1 + variableSetIndex);
        std::vector<MonteCarloEventChain> &eventChains = events.at(variableSetIndex);
        if(!accumulateRuns)
            eventChains.clear();
        size_t prevNumRuns = eventChains.size();
        eventChains.resize(prevNumRuns + numRuns);
        std::uniform_real_distribution<double> randomUniform(0, 1); // Uniform random numbers in [0, 1)
        double epsilon = std::numeric_limits<double>::epsilon() * 5;
        if(startEquilibrated)
            startingProbability = equilibriumProbability(epochs.begin()->transitionRates.toDense());
        Eigen::SparseMatrix<double> QT; // Transpose of Q matrix. !!! Needs to be updated for each epoch.
        for(size_t run = prevNumRuns; run < prevNumRuns + numRuns; ++run) {
            if(abort && *abort) return;
            MonteCarloEventChain &eventChain = eventChains.at(run);
            eventChain.reserve(1000);
            size_t eventCounter = 0;
            MonteCarloEvent event;
            // Set starting state.
            double prnd = randomUniform(randomNumberGenerator); // [0, 1)
            double ptot = 0;
            for(int i = 0; i < numStates; ++i) {
                ptot += startingProbability[i];
                if(ptot > prnd) {
                    event.state = i;
                    break;
                }
            }
            double eventChainDuration = 0;
            std::vector<Epoch>::iterator epochIter = epochs.begin();
            QT = epochIter->transitionRates.transpose();
            while(eventChainDuration < endTime) {
                if(abort && *abort) return;
                // Check if stuck in state.
                double kout = -epochIter->transitionRates.coeff(event.state, event.state); // Net rate leaving state.
                if(kout < epsilon) {
                    event.duration = endTime - eventChainDuration; // Remaining time.
                    eventChain.push_back(event);
                    break;
                }
                // Lifetime in state.
                double lifetime = epochIter->randomStateLifetimes[event.state](randomNumberGenerator);
                bool epochChanged = false;
                while(eventChainDuration + lifetime > epochIter->start + epochIter->duration) { // Event takes us to the next epoch.
                    // Truncate lifetime to end of epoch.
                    lifetime = epochIter->start + epochIter->duration - eventChainDuration;
                    // Go to next epoch.
                    ++epochIter;
                    if(epochIter == epochs.end())
                        break;
                    // Check if stuck in state.
                    kout = -epochIter->transitionRates.coeff(event.state, event.state); // Net rate leaving state.
                    if(kout < epsilon) {
                        epochIter = epochs.end();
                        break;
                    }
                    // Lifetime extends into new epoch.
                    lifetime += epochIter->randomStateLifetimes[event.state](randomNumberGenerator);
                    epochChanged = true;
                }
                // Check if we reached the end of the chain's duration.
                if(epochIter == epochs.end()) {
                    event.duration = endTime - eventChainDuration; // Remaining time.
                    eventChain.push_back(event);
                    break;
                }
                if(epochChanged)
                    QT = epochIter->transitionRates.transpose();
                // Add event to chain.
                event.duration = lifetime;
                eventChain.push_back(event);
                ++eventCounter;
                if(eventCounter == 1000) {
                    eventChain.reserve(eventChain.size() + 1000);
                    eventCounter = 0;
                }
                // Go to next state.
                if(eventChainDuration < endTime) {
                    // Select next state based on rates leaving current state.
                    prnd = randomUniform(randomNumberGenerator); // [0, 1)
                    ptot = 0;
                    for(Eigen::SparseMatrix<double>::InnerIterator it(QT, event.state); it; ++it) {
                        if(it.row() != event.state) {
                            ptot += it.value() / kout;
                            if(ptot >= prnd) {
                                event.state = it.row();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    void Simulation::getProbabilityFromEventChains(size_t numStates, size_t variableSetIndex, AbortFlag *abort, QString */* message */)
    {
        size_t numPts = time.size();
        while(probability.size() < variableSetIndex)
            probability.push_back(Eigen::MatrixXd::Zero(numPts, numStates));
        Eigen::MatrixXd &P = probability.at(variableSetIndex);
        P.setZero(numPts, numStates);
        const std::vector<MonteCarloEventChain> &eventChains = events.at(variableSetIndex);
        for(const MonteCarloEventChain &eventChain : eventChains) {
            size_t t = 0; // Index into time.
            MonteCarloEventChain::const_iterator eventIter = eventChain.begin();
            double sampleIntervalStart = time[t];
            double sampleIntervalEnd = t + 1 >= numPts ? endTime : time[t + 1];
            double sampleInterval = sampleIntervalEnd - sampleIntervalStart;
            double eventStart = 0;
            double eventEnd = eventStart + eventIter->duration;
            while(t < numPts && eventIter != eventChain.end()) {
                if(abort && *abort) return;
                if(eventStart <= sampleIntervalStart && eventEnd >= sampleIntervalEnd) {
                    // Event covers entire sample interval.
                    P(t, eventIter->state) += 1;
                    ++t;
                    sampleIntervalStart = sampleIntervalEnd;
                    sampleIntervalEnd = t + 1 < numPts ? time[t + 1] : endTime;
                    sampleInterval = sampleIntervalEnd - sampleIntervalStart;
                } else if(eventStart <= sampleIntervalStart) {
                    // Event stopped mid sample interval.
                    P(t, eventIter->state) += (eventEnd - sampleIntervalStart) / sampleInterval;
                    ++eventIter;
                    if(eventIter == eventChain.end()) break;
                    eventStart = eventEnd;
                    eventEnd = eventStart + eventIter->duration;
                } else if(eventEnd >= sampleIntervalEnd) {
                    // Event started mid sample interval.
                    P(t, eventIter->state) += (sampleIntervalEnd - eventStart) / sampleInterval;
                    ++t;
                    sampleIntervalStart = sampleIntervalEnd;
                    sampleIntervalEnd = t + 1 < numPts ? time[t + 1] : endTime;
                    sampleInterval = sampleIntervalEnd - sampleIntervalStart;
                } else {
                    // Event started and stopped mid sample interval.
                    P(t, eventIter->state) += eventIter->duration / sampleInterval;
                    ++eventIter;
                    if(eventIter == eventChain.end()) break;
                    eventStart = eventEnd;
                    eventEnd = eventStart + eventIter->duration;
                }
            }
        }
        P /= eventChains.size();
    }
    
    StimulusClampProtocol::StimulusClampProtocol(QObject *parent, const QString &name) :
    QObject(parent)
    {
        setName(name);
    }
    
    void StimulusClampProtocol::clear()
    {
        qDeleteAll(children());
        simulations.clear();
    }
    
    void StimulusClampProtocol::init(std::vector<Epoch*> &uniqueEpochs)
    {
        QList<Stimulus*> stimuli = findChildren<Stimulus*>(QString(), Qt::FindDirectChildrenOnly);
        
        // Parse conditions matrices.
        starts = str2mat<double>(start());
        durations = str2mat<double>(duration());
        sampleIntervals = str2mat<double>(sampleInterval());
        weights = str2mat<double>(weight());
        foreach(Stimulus *stimulus, stimuli) {
            if(stimulus->isActive()) {
                stimulus->starts = str2mat<double>(stimulus->start());
                stimulus->durations = str2mat<double>(stimulus->duration());
                stimulus->amplitudes = str2mat<double>(stimulus->amplitude());
                stimulus->onsetExprs = str2mat<std::string>(stimulus->onsetExpr());
                stimulus->offsetExprs = str2mat<std::string>(stimulus->offsetExpr());
                stimulus->repeats = str2mat<int>(stimulus->repetitions());
                stimulus->periods = str2mat<double>(stimulus->period());
            }
        }
        // Get max size for all conditions matrices.
        size_t rows = 1, cols = 1;
        matlims<double>(starts, &rows, &cols);
        matlims<double>(durations, &rows, &cols);
        matlims<double>(sampleIntervals, &rows, &cols);
        matlims<double>(weights, &rows, &cols);
        foreach(Stimulus *stimulus, stimuli) {
            if(stimulus->isActive()) {
                matlims<double>(stimulus->starts, &rows, &cols);
                matlims<double>(stimulus->durations, &rows, &cols);
                matlims<double>(stimulus->amplitudes, &rows, &cols);
                matlims<std::string>(stimulus->onsetExprs, &rows, &cols);
                matlims<std::string>(stimulus->offsetExprs, &rows, &cols);
                matlims<int>(stimulus->repeats, &rows, &cols);
                matlims<double>(stimulus->periods, &rows, &cols);
            }
        }
        // Pad all conditions matrices out to max size.
        padmat<double>(starts, rows, cols, 0);
        padmat<double>(durations, rows, cols, 0);
        padmat<double>(sampleIntervals, rows, cols, 0);
        padmat<double>(weights, rows, cols, 1);
        foreach(Stimulus *stimulus, stimuli) {
            if(stimulus->isActive()) {
                padmat<double>(stimulus->starts, rows, cols, 0);
                padmat<double>(stimulus->durations, rows, cols, 0);
                padmat<double>(stimulus->amplitudes, rows, cols, 0);
                padmat<std::string>(stimulus->onsetExprs, rows, cols, "");
                padmat<std::string>(stimulus->offsetExprs, rows, cols, "");
                padmat<int>(stimulus->repeats, rows, cols, 1);
                padmat<double>(stimulus->periods, rows, cols, 0);
            }
        }
        // Init simulations for each condition.
        simulations.resize(rows);
        for(size_t row = 0; row < rows; ++row) {
            simulations[row].resize(cols);
            for(size_t col = 0; col < cols; ++col) {
                Simulation &sim = simulations[row][col];
                // Sample time points.
                int numSteps = floor(durations[row][col] / sampleIntervals[row][col]);
                sim.time = Eigen::VectorXd::LinSpaced(1 + numSteps, starts[row][col], starts[row][col] + numSteps * sampleIntervals[row][col]);
                sim.endTime = starts[row][col] + durations[row][col];
                int numPts = sim.time.size();
                // Sample weights.
                sim.weight = Eigen::VectorXd::Constant(numPts, weights[row][col]);
                // Stimulus waveforms (plus weight and mask).
                Eigen::VectorXd mask = Eigen::VectorXd::Zero(numPts);
                foreach(Stimulus *stimulus, stimuli) {
                    if(stimulus->isActive()) {
                        if(stimulus->name().toLower() == "weight")
                            sim.weight += stimulus->waveform(sim.time, row, col);
                        else if(stimulus->name().toLower() == "mask")
                            mask += stimulus->waveform(sim.time, row, col);
                        else if(sim.stimuli.find(stimulus->name()) != sim.stimuli.end())
                            sim.stimuli[stimulus->name()] += stimulus->waveform(sim.time, row, col);
                        else
                            sim.stimuli[stimulus->name()] = stimulus->waveform(sim.time, row, col);
                    }
                }
                // Convert mask to boolean array. Any nonzero value will be set to false (i.e. masked).
                sim.mask = (mask.array() == 0);
                // Stimulus epochs.
                sim.findEpochsDiscretizedToSamplePoints();
                // Unique epochs.
                for(Epoch &epoch : sim.epochs) {
                    bool foundIt = false;
                    for(Epoch *uniqueEpoch : uniqueEpochs) {
                        if(uniqueEpoch->stimuli == epoch.stimuli) {
                            epoch.uniqueEpoch = uniqueEpoch;
                            foundIt = true;
                            break;
                        }
                    }
                    if(!foundIt) {
                        epoch.uniqueEpoch = new Epoch;
                        epoch.uniqueEpoch->stimuli = epoch.stimuli;
                        uniqueEpochs.push_back(epoch.uniqueEpoch);
                    }
                }
                // Random number generator.
                sim.randomNumberGenerator = getSeededRandomNumberGenerator<std::mt19937>();
            }
        }
    }
    
    double StimulusClampProtocol::cost()
    {
        double cost = 0;
        // ... TODO
        return cost;
    }
    
#ifdef DEBUG
    void StimulusClampProtocol::dump(std::ostream &out)
    {
        QVariantMap data = QObjectPropertyTreeSerializer::serialize(this, 1, true, false);
        out << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented).toStdString();
    }
#endif
    
    void Simulator::init()
    {
        model->init(stateNames);
        for(Epoch *epoch : uniqueEpochs) delete epoch;
        uniqueEpochs.clear();
        foreach(StimulusClampProtocol *protocol, protocols)
            protocol->init(uniqueEpochs);
    }
    
    void Simulator::run()
    {
        try {
            std::vector<QFuture<void> > futures;
            futures.reserve(100);
            for(size_t variableSetIndex = 0; variableSetIndex < model->numVariableSets(); ++variableSetIndex) {
                // Unique epochs.
                for(Epoch *epoch : uniqueEpochs) {
                    if(abort) break;
                    model->evalVariables(epoch->stimuli, variableSetIndex);
                    model->getStateProbabilities(epoch->stateProbabilities);
                    model->getStateAttributes(epoch->stateAttributes);
                    model->getTransitionRates(epoch->transitionRates);
                    model->getTransitionCharges(epoch->transitionCharges);
                    int numStates = epoch->transitionRates.cols();
                    if(options["Method"] == "Eigen Solver") {
                        std::function<void()> func = std::bind(spectralExpansion, std::ref(epoch->transitionRates), std::ref(epoch->spectralEigenValues), std::ref(epoch->spectralMatrices), &abort);
                        futures.push_back(QtConcurrent::run(func));
                    } else if(options["Method"] == "Monte Carlo") {
                        epoch->spectralEigenValues = Eigen::VectorXd::Zero(1);
                        epoch->spectralMatrices.clear();
                        epoch->randomStateLifetimes.clear();
                        epoch->randomStateLifetimes.reserve(numStates);
                        for(int i = 0; i < numStates; ++i)
                            epoch->randomStateLifetimes.push_back(std::exponential_distribution<double>(-epoch->transitionRates.coeff(i, i)));
                    }
                    if(epoch->transitionCharges.nonZeros())
                        epoch->stateChargeCurrents = epoch->transitionRates.cwiseProduct(epoch->transitionCharges).toDense().rowwise().sum() * 6.242e-6; // pA = 6.242e-6 e/s
                    else
                        epoch->stateChargeCurrents = Eigen::RowVectorXd::Zero(numStates);
                }
                for(QFuture<void> &future : futures)
                    future.waitForFinished();
                futures.clear();
                // Simulations.
                for(StimulusClampProtocol *protocol : protocols) {
                    for(size_t row = 0; row < protocol->simulations.size(); ++row) {
                        for(size_t col = 0; col < protocol->simulations[row].size(); ++col) {
                            if(abort) break;
                            Simulation &sim = protocol->simulations[row][col];
                            if(options["Method"] == "Eigen Solver") {
                                std::function<void()> func = std::bind(&Simulation::spectralSimulation, &sim, sim.epochs.begin()->uniqueEpoch->stateProbabilities, protocol->startEquilibrated(), variableSetIndex, &abort, &message);
                                futures.push_back(QtConcurrent::run(func));
                            } else if(options["Method"] == "Monte Carlo") {
                                std::function<void()> func = std::bind(&Simulation::monteCarloSimulation, &sim, sim.epochs.begin()->uniqueEpoch->stateProbabilities, std::ref(sim.randomNumberGenerator), options["# Monte Carlo Runs"].toInt(), options["Accumulate Monte Carlo Runs"].toBool(), protocol->startEquilibrated(), variableSetIndex, &abort, &message);
                                futures.push_back(QtConcurrent::run(func));
                            }
                        }
                    }
                }
                for(QFuture<void> &future : futures)
                    future.waitForFinished();
                futures.clear();
                // Waveforms.
                // Summaries.
            }
        } catch(std::runtime_error &e) {
            message = QString(e.what());
            abort = true;
            throw std::runtime_error(message.toStdString());
        } catch(...) {
            message = "Unknown error.";
            abort = true;
            throw std::runtime_error(message.toStdString());
        }
    }
    
    // Specialization for strings because ranges don't make sense for strings.
    template <>
    std::vector<std::string> str2vec<std::string>(const QString &str, const QString &delimiter, const QString &/* rangeDelimiter */)
    {
        std::vector<std::string> vec;
        QStringList fields = str.split(delimiter, QString::SkipEmptyParts);
        vec.reserve(fields.size());
        foreach(QString field, fields) {
            field = field.trimmed();
            if(!field.isEmpty()) {
                vec.push_back(field.toStdString());
            }
        }
        return vec;
    }
    
#ifdef DEBUG
#define VERIFY(x, errMsg) if(!(x)) { ++numErrors; std::cout << errMsg << std::endl; }
    void test()
    {
        int numErrors = 0;
        
        StimulusClampProtocol protocol;
        
        protocol.dump(std::cout);
        
        std::cout << "Test completed with " << numErrors << " error(s)." << std::endl;
    }
#endif

} // StimulusClampProtocol