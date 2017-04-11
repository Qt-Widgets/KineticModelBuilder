/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "MarkovModel.h"
#include "QObjectPropertyEditor.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <stdexcept>
#include <QJsonDocument>
#include <QVariantMap>

namespace MarkovModel
{
    QObjectPropertyTreeSerializer::ObjectFactory getObjectFactory()
    {
        QObjectPropertyTreeSerializer::ObjectFactory factory;
        factory.registerCreator("Variable", factory.defaultCreator<Variable>);
        factory.registerCreator("State", factory.defaultCreator<State>);
        factory.registerCreator("Transition", factory.defaultCreator<Transition>);
        factory.registerCreator("BinaryElement", factory.defaultCreator<BinaryElement>);
        factory.registerCreator("Interaction", factory.defaultCreator<Interaction>);
        factory.registerCreator("StateGroup", factory.defaultCreator<StateGroup>);
        factory.registerCreator("MarkovModel", factory.defaultCreator<MarkovModel>);
        return factory;
    }
    QObjectPropertyTreeSerializer::ObjectFactory MarkovModel::objectFactory = getObjectFactory();
    
    QStringList str2list(const QString &str, const QString &sep)
    {
        QStringList strs;
        QStringList fields = str.split(sep, QString::SkipEmptyParts);
        strs.reserve(fields.size());
        foreach(QString field, fields) {
            field = field.trimmed();
            if(!field.isEmpty())
                strs.push_back(field);
        }
        return strs;
    }
    
    std::map<QString, QString> str2exprMap(const QString &str)
    {
        std::map<QString, QString> attrs;
        QStringList fields = str.split(",", QString::SkipEmptyParts);
        foreach(QString field, fields) {
            QStringList subfields = field.split(":", QString::SkipEmptyParts);
            if(subfields.size() == 2) {
                QString attrName = subfields[0].trimmed();
                QString attrValue = subfields[1].trimmed();
                if(!attrName.isEmpty())
                    attrs[attrName] = attrValue;
            }
        }
        return attrs;
    }
    
    State::~State()
    {
        if(MarkovModel *model = qobject_cast<MarkovModel*>(parent())) {
            foreach(Transition *transition, model->findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(transition->from() == this || transition->to() == this)
                    delete transition;
            }
        }
    }
    
    void Transition::setFromName(QString s)
    {
        if(MarkovModel *model = qobject_cast<MarkovModel*>(parent()))
            setFrom(model->findChild<State*>(s, Qt::FindDirectChildrenOnly));
    }
    
    void Transition::setToName(QString s)
    {
        if(MarkovModel *model = qobject_cast<MarkovModel*>(parent()))
            setTo(model->findChild<State*>(s, Qt::FindDirectChildrenOnly));
    }
    
    BinaryElement::~BinaryElement()
    {
        if(MarkovModel *model = qobject_cast<MarkovModel*>(parent())) {
            foreach(Interaction *interaction, model->findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(interaction->A() == this || interaction->B() == this)
                    delete interaction;
            }
        }
    }
    void BinaryElement::getStatePairs(int elementIndex, int numStates,
                                      StateIndexPairs &stateIndexPairs01, StateIndexPairs &stateIndexPairs10)
    {
        stateIndexPairs01.clear();
        stateIndexPairs10.clear();
        int mask = (1 << elementIndex);
        for(int from = 0; from < numStates; ++from) {
            for(int to = 0; to < numStates; ++to) {
                if((from ^ to) == mask) {
                    if(to & mask)
                        stateIndexPairs01.push_back(StateIndexPair(from, to));
                    else
                        stateIndexPairs10.push_back(StateIndexPair(from, to));
                }
            }
        }
    }
    
    void BinaryElement::getConfigurationStateIndexes(const QString &config, std::vector<int> &stateIndexes)
    {
        stateIndexes.clear();
        stateIndexes.push_back(0);
        for(int i = 0; i < config.size(); ++i) {
            if(config[i] == '1') {
                for(size_t j = 0; j < stateIndexes.size(); ++j)
                    stateIndexes[j] ^= (1 << i);
            } else if(config[i] == '*') {
                size_t n = stateIndexes.size();
                for(size_t j = 0; j < n; ++j)
                    stateIndexes.push_back(stateIndexes[j] ^ (1 << i));
            } else if(config[i] != '0') {
                throw std::runtime_error("Invalid binary element configuration(s) '" + config.toStdString() + "'.");
            }
        }
    }
    
    void BinaryElement::getBinaryStateNames(int numBinaryElements, QStringList &stateNames)
    {
        int numStates = pow(2, numBinaryElements);
        stateNames.reserve(numStates);
        stateNames.clear();
        for(int i = 0; i < numStates; ++i) {
            QString name;
            name.fill(QChar('0'), numBinaryElements);
            for(int j = 0; j < numBinaryElements; ++j) {
                if(i & (1 << j))
                    name[j] = QChar('1');
            }
            stateNames.push_back(name);
        }
    }
    
    void Interaction::setAName(QString s)
    {
        if(MarkovModel *model = qobject_cast<MarkovModel*>(parent()))
            setA(model->findChild<BinaryElement*>(s, Qt::FindDirectChildrenOnly));
    }
    
    void Interaction::setBName(QString s)
    {
        if(MarkovModel *model = qobject_cast<MarkovModel*>(parent()))
            setB(model->findChild<BinaryElement*>(s, Qt::FindDirectChildrenOnly));
    }
    
    void Interaction::getStatePairs(int elementIndexA, int elementIndexB, int numStates,
                                    StateIndexPairs &stateIndexPairs1101, StateIndexPairs &stateIndexPairs1110,
                                    StateIndexPairs &stateIndexPairs0111, StateIndexPairs &stateIndexPairs1011)
    {
        stateIndexPairs1101.clear();
        stateIndexPairs1110.clear();
        stateIndexPairs0111.clear();
        stateIndexPairs1011.clear();
        int maskA = (1 << elementIndexA);
        int maskB = (1 << elementIndexB);
        for(int from = 0; from < numStates; ++from) {
            int configA = (from >> elementIndexA) & 1;
            int configB = (from >> elementIndexB) & 1;
            if(configA == 1 && configB == 1) {
                for(int to = 0; to < numStates; ++to) {
                    if((from ^ to) == maskA)
                        stateIndexPairs1101.push_back(StateIndexPair(from, to));
                    else if((from ^ to) == maskB)
                        stateIndexPairs1110.push_back(StateIndexPair(from, to));
                }
            } else if(configA == 1) {
                for(int to = 0; to < numStates; ++to) {
                    if((from ^ to) == maskB)
                        stateIndexPairs1011.push_back(StateIndexPair(from, to));
                }
            } else if(configB == 1) {
                for(int to = 0; to < numStates; ++to) {
                    if((from ^ to) == maskA)
                        stateIndexPairs0111.push_back(StateIndexPair(from, to));
                }
            }
        }
    }
    
    void StateGroup::getStateIndexes(const QString &states, const QStringList &stateNames, std::vector<int> &stateIndexes)
    {
        stateIndexes.clear();
        QStringList fields = states.split(",", QString::SkipEmptyParts);
        foreach(QString field, fields) {
            field = field.trimmed();
            if(!field.isEmpty()) {
                int index = stateNames.indexOf(field);
                if(index == -1)
                    throw std::runtime_error("Invalid state name '" + field.toStdString() + "'.");
                stateIndexes.push_back(index);
            }
        }
        std::sort(stateIndexes.begin(), stateIndexes.end()); // ascending order
        stateIndexes.resize(std::distance(stateIndexes.begin(), std::unique(stateIndexes.begin(), stateIndexes.end()))); // keep unique only
    }
    
    void StateGroup::getStateIndexes(const QString &configs, int numBinaryElements, std::vector<int> &stateIndexes)
    {
        stateIndexes.clear();
        std::vector<int> configStateIndexes;
        QStringList fields = configs.split(",", QString::SkipEmptyParts);
        foreach(QString field, fields) {
            field = field.trimmed();
            if(!field.isEmpty()) {
                if(field.size() != numBinaryElements)
                    throw std::runtime_error("Invalid number of elements in configuration '" + field.toStdString() + "'.");
                BinaryElement::getConfigurationStateIndexes(field, configStateIndexes);
                if(!configStateIndexes.empty())
                    stateIndexes.insert(stateIndexes.end(), configStateIndexes.begin(), configStateIndexes.end());
            }
        }
        std::sort(stateIndexes.begin(), stateIndexes.end()); // ascending order
        stateIndexes.resize(std::distance(stateIndexes.begin(), std::unique(stateIndexes.begin(), stateIndexes.end()))); // keep unique only
    }
    
    MarkovModel::MarkovModel(QObject *parent, const QString &name) :
    QObject(parent)
    {
        setName(name);
        
        // Default model.
        new Variable(this, "k", "0.000086173324", "Boltzmann constant (eV/K)");
        new Variable(this, "R", "0.0019872036", "Gas constant (kcal/mol/K)");
        new Variable(this, "h", "4.135667662*10^-15", "Plank constant (eV*s)");
        State *A = new State(this, "A");
        State *B = new State(this, "B");
        new Transition(this, A, B);
        new Transition(this, B, A);
        A->setProbability("1");
        A->position = QVector3D(-2, 0, 0);
        B->position = QVector3D(+2, 0, 0);
    }
    
    Transition* MarkovModel::findTransition(State *from, State *to)
    {
        foreach(Transition *transition, findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly)) {
            if(transition->from() == from && transition->to() == to)
                return transition;
        }
        return 0;
    }
    
    Interaction* MarkovModel::findInteraction(BinaryElement *A, BinaryElement *B)
    {
        foreach(Interaction *interaction, findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly)) {
            if((interaction->A() == A && interaction->B() == B) || (interaction->A() == B && interaction->B() == A))
                return interaction;
        }
        return 0;
    }
    
    void MarkovModel::clear()
    {
        qDeleteAll(findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly));
        qDeleteAll(findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly));
        qDeleteAll(children());
    }
    
    void MarkovModel::init(QStringList &stateNames)
    {
        std::map<QString, int> occurances;
        QList<Variable*> variables = findChildren<Variable*>(QString(), Qt::FindDirectChildrenOnly);
        foreach(Variable *variable, variables) {
            if(occurances.find(variable->name()) == occurances.end()) {
                variable->setIndex(0);
                occurances[variable->name()] = 1;
            } else {
                variable->setIndex(occurances[variable->name()]);
                occurances[variable->name()] += 1;
            }
        }
        foreach(Variable *variable, variables)
        variable->setNumIndexes(occurances[variable->name()]);
        QList<BinaryElement*> binaryElements = findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly);
        int numBinaryElements = binaryElements.size();
        if(numBinaryElements) {
            int numStates = pow(2, numBinaryElements);
            int elementIndex = 0;
            foreach(BinaryElement *element, binaryElements) {
                element->setIndex(elementIndex++);
                BinaryElement::getStatePairs(element->index(), numStates,
                                             element->stateIndexPairs01, element->stateIndexPairs10);
            }
            foreach(Interaction *interaction, findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(interaction->A() && interaction->B()) {
                    Interaction::getStatePairs(interaction->A()->index(), interaction->B()->index(), numStates,
                                               interaction->stateIndexPairs1101, interaction->stateIndexPairs1110,
                                               interaction->stateIndexPairs0111, interaction->stateIndexPairs1011);
                }
            }
            BinaryElement::getBinaryStateNames(numBinaryElements, stateNames);
        } else {
            int stateIndex = 0;
            QList<State*> states = findChildren<State*>(QString(), Qt::FindDirectChildrenOnly);
            stateNames.reserve(states.size());
            stateNames.clear();
            foreach(State *state, states) {
                state->setIndex(stateIndex++);
                stateNames.push_back(state->name());
            }
        }
        foreach(StateGroup *group, findChildren<StateGroup*>(QString(), Qt::FindDirectChildrenOnly)) {
            if(group->isActive()) {
                if(numBinaryElements)
                    StateGroup::getStateIndexes(group->states(), numBinaryElements, group->stateIndexes);
                else
                    StateGroup::getStateIndexes(group->states(), stateNames, group->stateIndexes);
            }
        }
    }
    
    void MarkovModel::evalVariables(const ParameterMap &stimuli, size_t variableSetIndex)
    {
        parameters = stimuli;
#ifdef USE_EXPR_TK
        _symbols.clear();
        //_symbols.add_constants();
#else
        _parser.vars().clear();
#endif
        for(ParameterMap::const_iterator it = stimuli.begin(); it != stimuli.end(); ++it) {
            std::string stimulusName = it->first.trimmed().toStdString();
            double stimulusValue = it->second;
#ifdef USE_EXPR_TK
            _symbols.add_variable(stimulusName, stimulusValue);
#else
            _parser.var(stimulusName) = stimulusValue;
#endif
        }
#ifdef USE_EXPR_TK
        _expr.register_symbol_table(_symbols);
#endif
        foreach(Variable *variable, findChildren<Variable*>(QString(), Qt::FindDirectChildrenOnly)) {
            if((variable->index() == variableSetIndex) || (variable->index() < variableSetIndex && variable->numIndexes() <= variableSetIndex)) {
                double value = evalExpr(variable->value());
                parameters[variable->name()] = value;
#ifdef USE_EXPR_TK
                _symbols.add_variable(variable->name().toStdString(), value);
                _expr.register_symbol_table(_symbols);
#else
                _parser.var(variable->name().toStdString()) = value;
#endif
            }
        }
    }
    
    size_t MarkovModel::numVariableSets()
    {
        size_t numSets = 0;
        foreach(Variable *variable, findChildren<Variable*>(QString(), Qt::FindDirectChildrenOnly)) {
            if(variable->numIndexes() > numSets)
                numSets = variable->numIndexes();
        }
        return numSets;
    }
    
    void MarkovModel::getStateProbabilities(Eigen::RowVectorXd &stateProbabilities)
    {
        QList<BinaryElement*> binaryElements = findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly);
        int numBinaryElements = binaryElements.size();
        if(numBinaryElements) {
            int numStates = pow(2, numBinaryElements);
            stateProbabilities = Eigen::RowVectorXd::Ones(numStates);
            Eigen::VectorXd binaryElementProbabilities0 = Eigen::VectorXd::Ones(numBinaryElements);
            for(int j = 0; j < numBinaryElements; ++j) {
                double probability0 = evalExpr(binaryElements[j]->probability0());
                if(probability0 != 1)
                    binaryElementProbabilities0[j] = probability0 < 0 ? 0 : (probability0 > 1 ? 1 : probability0);
            }
            for(int i = 0; i < numStates; ++i) {
                for(int j = 0; j < numBinaryElements; ++j) {
                    if(i & (1 << j))
                        stateProbabilities[i] *= (1 - binaryElementProbabilities0[j]);
                    else
                        stateProbabilities[i] *= binaryElementProbabilities0[j];
                }
            }
        } else {
            QList<State*> states = findChildren<State*>(QString(), Qt::FindDirectChildrenOnly);
            stateProbabilities = Eigen::RowVectorXd::Zero(states.size());
            int i = 0;
            foreach(State *state, states) {
                double probability = evalExpr(state->probability());
                if(probability)
                    stateProbabilities[i] = probability < 0 ? 0 : (probability > 1 ? 1 : probability);
                ++i;
            }
            double totalProbability = stateProbabilities.sum();
            if(totalProbability < 1e-5)
                throw std::runtime_error("At least one state must have non-zero starting probability.");
            else if(totalProbability != 1)
                stateProbabilities /= totalProbability;
        }
    }
    
    void MarkovModel::getStateAttributes(std::map<QString, Eigen::RowVectorXd> &stateAttributes)
    {
        QList<BinaryElement*> binaryElements = findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly);
        QList<State*> states;
        int numBinaryElements = binaryElements.size();
        int numStates = 0;
        if(numBinaryElements) {
            numStates = pow(2, numBinaryElements);
        } else {
            states = findChildren<State*>(QString(), Qt::FindDirectChildrenOnly);
            numStates = states.size();
        }
        foreach(StateGroup *stateGroup, findChildren<StateGroup*>(QString(), Qt::FindDirectChildrenOnly)) {
            if(stateGroup->isActive()) {
                std::map<QString, QString> attrExprs = str2exprMap(stateGroup->attributes());
                for(std::map<QString, QString>::iterator it = attrExprs.begin(); it != attrExprs.end(); ++it) {
                    QString attrName = it->first;
                    double attrValue = evalExpr(it->second);
                    if(stateAttributes.find(attrName) == stateAttributes.end())
                        stateAttributes[attrName] = Eigen::RowVectorXd::Zero(numStates);
                    if(attrValue) {
                        Eigen::RowVectorXd &stateAttrs = stateAttributes[attrName];
                        for(int stateIndex : stateGroup->stateIndexes)
                            stateAttrs[stateIndex] = attrValue;
                    }
                }
            }
        }
        if(numBinaryElements == 0) {
            // state attribute overrides group attribute
            int stateIndex = 0;
            foreach(State *state, states) {
                std::map<QString, QString> attrExprs = str2exprMap(state->attributes());
                for(std::map<QString, QString>::iterator it = attrExprs.begin(); it != attrExprs.end(); ++it) {
                    QString attrName = it->first;
                    double attrValue = evalExpr(it->second);
                    if(stateAttributes.find(attrName) == stateAttributes.end())
                        stateAttributes[attrName] = Eigen::RowVectorXd::Zero(numStates);
                    if(attrValue)
                        stateAttributes[attrName][stateIndex] = attrValue;
                }
                ++stateIndex;
            }
        }
    }
    
    void MarkovModel::getTransitionRates(Eigen::SparseMatrix<double> &transitionRates)
    {
        QList<BinaryElement*> binaryElements = findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly);
        int numBinaryElements = binaryElements.size();
        if(numBinaryElements) {
            int numStates = pow(2, numBinaryElements);
            transitionRates.setZero();
            transitionRates.resize(numStates, numStates);
            foreach(BinaryElement *binaryElement, binaryElements) {
                double rate01 = evalExpr(binaryElement->rate01());
                double rate10 = evalExpr(binaryElement->rate10());
                if(rate01 < 0)
                    throw std::runtime_error("Negative transition rate: '" + binaryElement->rate01().toStdString() + "'");
                if(rate10 < 0)
                    throw std::runtime_error("Negative transition rate: '" + binaryElement->rate10().toStdString() + "'");
                if(rate01 > 0) {
                    for(const BinaryElement::StateIndexPair &fromTo : binaryElement->stateIndexPairs01)
                        transitionRates.insert(fromTo.first, fromTo.second) = rate01;
                }
                if(rate10 > 0) {
                    for(const BinaryElement::StateIndexPair &fromTo : binaryElement->stateIndexPairs10)
                        transitionRates.insert(fromTo.first, fromTo.second) = rate10;
                }
            }
            // Apply interaction multiplicative factors to all transitions where an element
            // involved in an interaction changed configuration.
            foreach(Interaction *interaction, findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(interaction->A() && interaction->B()) {
                    double factor11 = evalExpr(interaction->factor11());
                    double factorA1 = evalExpr(interaction->factorA1());
                    double factor1B = evalExpr(interaction->factor1B());
                    if(factor11 < 0)
                        throw std::runtime_error("Negative interaction factor: '" + interaction->factor11().toStdString() + "'");
                    if(factorA1 < 0)
                        throw std::runtime_error("Negative interaction factor: '" + interaction->factorA1().toStdString() + "'");
                    if(factor1B < 0)
                        throw std::runtime_error("Negative interaction factor: '" + interaction->factor1B().toStdString() + "'");
                    if(factorA1 != 1) {
                        for(const Interaction::StateIndexPair &fromTo : interaction->stateIndexPairs0111)
                            transitionRates.coeffRef(fromTo.first, fromTo.second) *= factorA1;
                    }
                    if(factor1B != 1) {
                        for(const Interaction::StateIndexPair &fromTo : interaction->stateIndexPairs1011)
                            transitionRates.coeffRef(fromTo.first, fromTo.second) *= factor1B;
                    }
                    if(factorA1 / factor11 != 1) {
                        for(const Interaction::StateIndexPair &fromTo : interaction->stateIndexPairs1101)
                            transitionRates.coeffRef(fromTo.first, fromTo.second) *= (factorA1 / factor11);
                    }
                    if(factor1B / factor11 != 1) {
                        for(const Interaction::StateIndexPair &fromTo : interaction->stateIndexPairs1110)
                            transitionRates.coeffRef(fromTo.first, fromTo.second) *= (factor1B / factor11);
                    }
                }
            }
        } else {
            QList<State*> states = findChildren<State*>(QString(), Qt::FindDirectChildrenOnly);
            int numStates = states.size();
            transitionRates.setZero();
            transitionRates.resize(numStates, numStates);
            foreach(Transition *transition, findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(transition->from() && transition->to()) {
                    double rate = evalExpr(transition->rate());
                    if(rate < 0)
                        throw std::runtime_error("Negative transition rate: '" + transition->rate().toStdString() + "'");
                    if(rate > 0)
                        transitionRates.insert(transition->from()->index(), transition->to()->index()) = rate;
                }
            }
        }
        // Set the diagonal elements so transition rates matrix is unitary (probability conservation).
        for(int i = 0; i < transitionRates.rows(); i++)
            transitionRates.coeffRef(i, i) = -transitionRates.row(i).sum();
    }
    
    void MarkovModel::getTransitionCharges(Eigen::SparseMatrix<double> &transitionCharges)
    {
        QList<BinaryElement*> binaryElements = findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly);
        int numBinaryElements = binaryElements.size();
        if(numBinaryElements) {
            int numStates = pow(2, numBinaryElements);
            transitionCharges.setZero();
            transitionCharges.resize(numStates, numStates);
            foreach(BinaryElement *binaryElement, binaryElements) {
                double charge01 = evalExpr(binaryElement->charge01());
                double charge10 = evalExpr(binaryElement->charge10());
                if(charge01) {
                    for(const BinaryElement::StateIndexPair &fromTo : binaryElement->stateIndexPairs01)
                        transitionCharges.insert(fromTo.first, fromTo.second) = charge01;
                }
                if(charge10) {
                    for(const BinaryElement::StateIndexPair &fromTo : binaryElement->stateIndexPairs10)
                        transitionCharges.insert(fromTo.first, fromTo.second) = charge10;
                }
            }
        } else {
            QList<State*> states = findChildren<State*>(QString(), Qt::FindDirectChildrenOnly);
            int numStates = states.size();
            transitionCharges.setZero();
            transitionCharges.resize(numStates, numStates);
            foreach(Transition *transition, findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(transition->from() && transition->to()) {
                    double charge = evalExpr(transition->charge());
                    if(charge)
                        transitionCharges.insert(transition->from()->index(), transition->to()->index()) = charge;
                }
            }
        }
    }
    
    double MarkovModel::evalExpr(const QString &expr)
    {
        if(expr.isEmpty())
            return 0;
#ifdef USE_EXPR_TK
        if(!_parser.compile(expr.toStdString(), _expr))
            throw std::runtime_error("Failed to parse '" + expr.toStdString() + "': " + _parser.error());
        return _expr.value();
#else
        EigenLab::ValueXd result = _parser.eval(expr.toStdString());
        if(result.matrix().size() != 1)
            throw std::runtime_error("Failed to reduce '" + expr.toStdString() + "' to a single number.");
        return result.matrix()(0, 0);
#endif
    }
    
    void MarkovModel::getFreeVariables(std::vector<double> &values, std::vector<double> &min, std::vector<double> &max)
    {
        values.clear();
        min.clear();
        max.clear();
        foreach(Variable *variable, findChildren<Variable*>(QString(), Qt::FindDirectChildrenOnly)) {
            if(!variable->isConst()) {
                double value = variable->number();
                if(variable->isNumber()) {
                    values.push_back(value);
                    min.push_back(variable->min());
                    max.push_back(variable->max());
                }
            }
        }
    }
    
    void MarkovModel::setFreeVariables(const std::vector<double> &values)
    {
        std::vector<double>::const_iterator it = values.begin();
        foreach(Variable *variable, findChildren<Variable*>(QString(), Qt::FindDirectChildrenOnly)) {
            if(!variable->isConst() && variable->isNumber()) {
                if(it == values.end())
                    throw std::runtime_error("MarkovModel::setFreeVariables: Too few values supplied.");
                variable->setValue(QString::number(*it));
                ++it;
            }
        }
    }
    
#ifdef DEBUG
    void MarkovModel::dump(std::ostream &out)
    {
        QVariantMap data = QObjectPropertyTreeSerializer::serialize(this, 1, true, false);
        out << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented).toStdString();
#ifdef USE_EXPR_TK
#else
        out << "Parameters:" << std::endl;
        for(auto &kv : _parser.vars())
            out << kv.first << " = " << kv.second.matrix() << std::endl;
#endif
    }
#endif
    
#ifdef DEBUG
#define VERIFY(x, errMsg) if(!(x)) { ++numErrors; std::cout << errMsg << std::endl; }
    void test()
    {
        int numErrors = 0;
        
        MarkovModel model;
        Variable *x = new Variable(&model, "x");
        Variable *y = new Variable(&model, "y");
        State *A = new State(&model, "A");
        State *B = new State(&model, "B");
        Transition *AB = new Transition(&model, A, B);
        Transition *BA = new Transition(&model, B, A);
        
        x->setValue("3.14 * z");
        y->setValue("sqrt(((2 + 0) * -3.14)^2)"); // 6.28
        A->setProbability("1");
        B->setAttributes("g: 15 * 1 + (0 * 7)^3, F: -100.1 - 0 / sqrt(9.45)"); // g: 15, F: -100.1
        AB->setRate("x"); // 3.14 z
        BA->setRate("y/2"); // 3.14
        AB->setCharge("x*0"); // 0
        BA->setCharge("y- y"); // 0
        
        {
            std::cout << "Checking states model..." << std::endl;
            
            QStringList stateNames;
            model.init(stateNames);
            VERIFY(stateNames == (QStringList() << "A" << "B"), "Invalid state names.");
            
            std::map<QString, double> stimuli;
            stimuli["z"] = 1;
            model.evalVariables(stimuli);
            VERIFY(model.parameters["z"] == 1, "Invalid parameter z.");
            VERIFY(model.parameters["x"] == 3.14*1, "Invalid parameter x.");
            VERIFY(model.parameters["y"] == sqrt(pow((2 + 0) * -3.14, 2)), "Invalid parameter y.");
            stimuli["z"] = 3;
            model.evalVariables(stimuli);
            VERIFY(model.parameters["z"] == 3, "Invalid parameter z.");
            VERIFY(model.parameters["x"] == 3.14*3, "Invalid parameter x.");
            VERIFY(model.parameters["y"] == sqrt(pow((2 + 0) * -3.14, 2)), "Invalid parameter y.");
            
            double vz = 3, vx = 3.14*vz, vy = sqrt(pow((2 + 0) * -3.14, 2));
            
            Eigen::RowVectorXd P, _P(2);
            model.getStateProbabilities(P);
            _P << 1, 0;
            VERIFY(P.isApprox(_P), "Invalid state probabilities.");
            
            std::map<QString, Eigen::RowVectorXd> attrs;
            model.getStateAttributes(attrs);
            VERIFY(attrs.size() == 2, "Wrong # of state attributes.");
            Eigen::RowVectorXd _g(2), _F(2);
            _g << 0, 15 * 1 + pow(0 * 7, 3);
            _F << 0, -100.1 - 0 / sqrt(9.45);
            VERIFY(attrs["g"].isApprox(_g), "Invalid state g.");
            VERIFY(attrs["F"].isApprox(_F), "Invalid state F.");
            
            Eigen::SparseMatrix<double> Q;
            model.getTransitionRates(Q);
            Eigen::MatrixXd _Q(2, 2);
            double kAB = vx, kBA = vy/2;
            _Q <<  -kAB,  kAB,
                    kBA, -kBA;
            VERIFY(Q.toDense().isApprox(_Q), "Invalid transition rates.");
            
            Eigen::SparseMatrix<double> Qc;
            model.getTransitionCharges(Qc);
            Eigen::MatrixXd _Qc(2, 2);
            _Qc <<  0, 0,
                    0, 0;
            VERIFY(Qc.toDense().isApprox(_Qc), "Invalid transition charges.");
        }
        
        BinaryElement *C = new BinaryElement(&model, "C");
        BinaryElement *D = new BinaryElement(&model, "D");
        Interaction *CD = new Interaction(&model, C, D);
        StateGroup *G = new StateGroup(&model, "G");
        
        C->setRate01("x"); // 3.14 z
        C->setRate10("y/2"); // 3.14
        C->setCharge01("x"); // 3.14 z
        C->setCharge10("-x"); // -3.14 z
        D->setRate01("x/2*z"); // 1.57 z^2
        D->setRate10("y"); // 6.28
        D->setCharge01(""); // 0
        D->setCharge10(""); // 0
        CD->setFactor11("2"); // 2
        CD->setFactorA1("10*y"); // 62.8
        CD->setFactor1B("y / 10"); // 0.628
        G->setStates("*1");
        G->setAttributes("g: 15.0, F: 100");
        
        {
            std::cout << "Checking binary elements model..." << std::endl;
            
            QStringList stateNames;
            model.init(stateNames);
            VERIFY(stateNames == (QStringList() << "00" << "10" << "01" << "11"), "Invalid state names.");
            
            std::map<QString, double> stimuli;
            stimuli["z"] = 1;
            model.evalVariables(stimuli);
            VERIFY(model.parameters["z"] == 1, "Invalid parameter z.");
            VERIFY(model.parameters["x"] == 3.14*1, "Invalid parameter x.");
            VERIFY(model.parameters["y"] == sqrt(pow((2 + 0) * -3.14, 2)), "Invalid parameter y.");
            stimuli["z"] = 3;
            model.evalVariables(stimuli);
            VERIFY(model.parameters["z"] == 3, "Invalid parameter z.");
            VERIFY(model.parameters["x"] == 3.14*3, "Invalid parameter x.");
            VERIFY(model.parameters["y"] == sqrt(pow((2 + 0) * -3.14, 2)), "Invalid parameter y.");
            
            double vz = 3, vx = 3.14*vz, vy = sqrt(pow((2 + 0) * -3.14, 2));
            
            Eigen::RowVectorXd P, _P(4);
            model.getStateProbabilities(P);
            _P << 1, 0, 0, 0;
            VERIFY(P.isApprox(_P), "Invalid state probabilities.");
            
            std::map<QString, Eigen::RowVectorXd> attrs;
            model.getStateAttributes(attrs);
            VERIFY(attrs.size() == 2, "Wrong # of state attributes.");
            Eigen::RowVectorXd _g(4), _F(4);
            _g << 0, 0, 15.0, 15.0;
            _F << 0, 0, 100, 100;
            VERIFY(attrs["g"].isApprox(_g), "Invalid state g.");
            VERIFY(attrs["F"].isApprox(_F), "Invalid state F.");
            
            Eigen::SparseMatrix<double> Q;
            model.getTransitionRates(Q);
            Eigen::MatrixXd _Q(4, 4);
            double C01 = vx, C10 = vy/2, D01 = vx/2*vz, D10 = vy;
            double C1F1 = 2, CF1 = 10*vy, C1F = vy/10;
            _Q << -C01-D01,  C01,           D01,           0,
            C10,    -C10-D01*C1F,   0,             D01*C1F,
            D10,     0,            -D10-C01*CF1,   C01*CF1,
            0,       D10*C1F/C1F1,  C10*CF1/C1F1, -D10*C1F/C1F1-C10*CF1/C1F1;
            VERIFY(Q.toDense().isApprox(_Q), "Invalid transition rates.");
            
            Eigen::SparseMatrix<double> Qc;
            model.getTransitionCharges(Qc);
            Eigen::MatrixXd _Qc(4, 4);
            _Qc <<   0,    9.42,  0,    0,
                    -9.42, 0,     0,    0,
                     0,    0,     0,    9.42,
                     0,    0,    -9.42, 0;
            VERIFY(Qc.toDense().isApprox(_Qc), "Invalid transition charges.");
        }
        
        model.dump(std::cout);
        
        std::cout << "Test completed with " << numErrors << " error(s)." << std::endl;
    }
#endif

} // MarkovModel
