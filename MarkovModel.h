/* --------------------------------------------------------------------------------
 * Markov model representation.
 *
 * A model is an object tree whose child objects represent states/transitions,
 * binary elements/interactions, state groups and variables.
 *
 * Most object parameters are specified as a string expression that can refer to
 * named variables (either model variables or external stimuli).
 * These expressions are evaluated by the model to obtain their numeric values.
 *
 * Variable values can also be expressions that depend on other variables. These are
 * evaluated in order, so the order of the model's list of varibles may matter.
 *
 * For model optimization, all variables whose value expressions are a simple number,
 * (and which are not held constant) are allowed to vary within their specified
 * [min, max] bounds.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __MarkovModel_H__
#define __MarkovModel_H__

#include <map>
#include <vector>
#include <QObject>
#include <QString>
#include <QVector3D>
#include <QWidget>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#ifdef USE_EXPR_TK
#include "exprtk.hpp"
#else
#include "EigenLab.h"
#endif
#include "QObjectPropertyTreeSerializer.h"
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace MarkovModel
{
    // For dynamic object creation.
    QObjectPropertyTreeSerializer::ObjectFactory getObjectFactory();
    
    // Parse a comma-separated list of strings.
    QStringList str2list(const QString &str, const QString &sep = ",");
    
    // Parse a string rep of a dict of named expressions.
    // e.g. Used to parse State/StateGroup attributes.
    std::map<QString, QString> str2exprMap(const QString &str);
    
    /* --------------------------------------------------------------------------------
     * Named value expression optionally allowed to vary within bounds.
     * - Value is a math expression that may refer to other variables by name.
     *   This expression will be parsed by the parent model to obtain the variable's
     *   numeric value. The model's variables are parsed in order, so order may matter.
     * - When value denotes a single number, then Const indicates whether or not it is
     *   held constant or allowed to vary within bounds [Min, Max].
     *   !!! Note that when Value is a more complex math expression, its numeric value
     *       is obtained by evaluating the expression (i.e. it is no longer a free variable
     *       regardless of what Const or Min/Max are set to).
     * - Index is used to keep track of which set the variable belongs to in the case
     *   of multiple variables with the same name.
     * - NumIndexes keeps track of how many variables share this variable's name.
     * - IsNumber is for convenience to keep track of whether the value expression denotes
     *   a single number or not.
     * -------------------------------------------------------------------------------- */
    class Variable : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(QString Value READ value WRITE setValue)
        Q_PROPERTY(QString Description READ description WRITE setDescription)
        Q_PROPERTY(bool Const READ isConst WRITE setIsConst)
        Q_PROPERTY(double Min READ min WRITE setMin)
        Q_PROPERTY(double Max READ max WRITE setMax)
        
    public:
        // Default constructor.
        Variable(QObject *parent = 0, const QString &name = "", const QString &value = "", const QString &description = "") :
        QObject(parent), _isConst(true), _min(0), _max(0), _index(0), _numIndexes(1), _isNumber(false) { setName(name); setValue(value); setDescription(description); }
        
        // Property getters.
        QString name() const { return objectName(); }
        QString value() const { return _value; }
        QString description() const { return _description; }
        bool isConst() const { return _isConst; }
        double min() const { return _min; }
        double max() const { return _max; }
        size_t index() const { return _index; }
        size_t numIndexes() const { return _numIndexes; }
        double number() { return _value.toDouble(&_isNumber); }
        bool isNumber() { return _isNumber; } // Only valid after number() is called.
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setValue(QString s) { _value = s; }
        void setDescription(QString s) { _description = s; }
        void setIsConst(bool b) { _isConst = b; }
        void setMin(double d) { _min = d; }
        void setMax(double d) { _max = d; }
        void setIndex(size_t i) { _index = i; }
        void setNumIndexes(size_t i) { _numIndexes = i; }
        
    protected:
        // Properties.
        QString _value;
        QString _description;
        bool _isConst;
        double _min;
        double _max;
        size_t _index;
        size_t _numIndexes;
        bool _isNumber;
    };
    
    /* --------------------------------------------------------------------------------
     * State of the system.
     * - Probability is a math expression that may refer to variables and/or external
     *   stimuli by name.
     * - Attributes is a dict-like repr of named math expressions that may each refer to
     *   variables and/or external stimuli by name.
     * - Position is used only for graphical representation of the parent model.
     * -------------------------------------------------------------------------------- */
    class State : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(QString Probability READ probability WRITE setProbability)
        Q_PROPERTY(QString Attributes READ attributes WRITE setAttributes)
        Q_PROPERTY(float X READ x WRITE setX)
        Q_PROPERTY(float Y READ y WRITE setY)
        Q_PROPERTY(float Z READ z WRITE setZ)
        
    public:
        // Default constructor.
        State(QObject *parent = 0, const QString &name = "") :
        QObject(parent), _probability("0"), _index(-1) { setName(name); }
        
        // Delete any connected transitions in the parent model upon destruction.
        ~State();
        
        // Property getters.
        QString name() const { return objectName(); }
        QString probability() const { return _probability; }
        QString attributes() const { return _attributes; }
        float x() const { return position.x(); }
        float y() const { return position.y(); }
        float z() const { return position.z(); }
        int index() const { return _index; } // Index into parent model's list of states.
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setProbability(QString s) { _probability = s; }
        void setAttributes(QString s) { _attributes = s; }
        void setX(float x) { position.setX(x); }
        void setY(float y) { position.setY(y); }
        void setZ(float z) { position.setZ(z); }
        void setIndex(int i) { _index = i; }
        
        // Position (x, y, z).
        QVector3D position;
        
    protected:
        // Properties.
        QString _probability;
        QString _attributes;
        int _index;
    };
    
    /* --------------------------------------------------------------------------------
     * Directed transition between two states.
     * - Rate and Charge are arbitrary math expressions that may refer to variables and/or
     *   external stimuli by name.
     * -------------------------------------------------------------------------------- */
    class Transition : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString From READ fromName WRITE setFromName)
        Q_PROPERTY(QString To READ toName WRITE setToName)
        Q_PROPERTY(QString Rate READ rate WRITE setRate)
        Q_PROPERTY(QString Charge READ charge WRITE setCharge)
        
    public:
        // Default constructor.
        Transition(QObject *parent = 0, State *from = 0, State *to = 0) :
        QObject(parent), _from(from), _to(to), _rate("10"), _charge("0") { updateName(); }
        
        // Property getters.
        State* from() const { return _from; }
        State* to() const { return _to; }
        QString fromName() const { return _from ? _from->name() : QString(); }
        QString toName() const { return _to ? _to->name() : QString(); }
        QString rate() const { return _rate; }
        QString charge() const { return _charge; }
        
        // Property setters.
        void setFrom(State *from) { _from = from; updateName(); }
        void setTo(State *to) { _to = to; updateName(); }
        void setFromName(QString s);
        void setToName(QString s);
        void setRate(QString s) { _rate = s; }
        void setCharge(QString s) { _charge = s; }
        
        // Name reflects connected state names.
        void updateName() { setObjectName(fromName() + " -> " + toName()); }
        
    protected:
        // Properties.
        State *_from;
        State *_to;
        QString _rate;
        QString _charge;
    };
    
    /* --------------------------------------------------------------------------------
     * System element that can transition between two possible configurations.
     * - Probability0, Rate01, Rate10, Charge01 and Charge10 are math expressions that
     *   may refer to variables and/or external stimuli by name.
     * - *0 and *1 refer to configurations 0 and 1, respectively.
     *   *01 and *10 refer to transitions from configurations 0->1 and 1->0, respectively.
     * - Position is used only for graphical representation of the parent model.
     * -------------------------------------------------------------------------------- */
    class BinaryElement : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(QString Probability0 READ probability0 WRITE setProbability0)
        Q_PROPERTY(QString Rate01 READ rate01 WRITE setRate01)
        Q_PROPERTY(QString Rate10 READ rate10 WRITE setRate10)
        Q_PROPERTY(QString Charge01 READ charge01 WRITE setCharge01)
        Q_PROPERTY(QString Charge10 READ charge10 WRITE setCharge10)
        Q_PROPERTY(float X READ x WRITE setX)
        Q_PROPERTY(float Y READ y WRITE setY)
        Q_PROPERTY(float Z READ z WRITE setZ)
        
    public:
        // Default constructor.
        BinaryElement(QObject *parent = 0, const QString &name = "") :
        QObject(parent), _probability0("1"), _rate01("10"), _rate10("10"), _charge01("0"), _charge10("0"), _index(-1) { setName(name); }
        
        // Delete any connected interactions in the parent model upon destruction.
        ~BinaryElement();
        
        // Property getters.
        QString name() const { return objectName(); }
        QString probability0() const { return _probability0; }
        QString rate01() const { return _rate01; }
        QString rate10() const { return _rate10; }
        QString charge01() const { return _charge01; }
        QString charge10() const { return _charge10; }
        float x() const { return position.x(); }
        float y() const { return position.y(); }
        float z() const { return position.z(); }
        int index() const { return _index; } // Index into parent model's list of binary elements.
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setProbability0(QString s) { _probability0 = s; }
        void setRate01(QString s) { _rate01 = s; }
        void setRate10(QString s) { _rate10 = s; }
        void setCharge01(QString s) { _charge01 = s; }
        void setCharge10(QString s) { _charge10 = s; }
        void setX(float x) { position.setX(x); }
        void setY(float y) { position.setY(y); }
        void setZ(float z) { position.setZ(z); }
        void setIndex(int i) { _index = i; }
        
        // Position (x, y, z).
        QVector3D position;
        
        // Lists of (from, to) state indexes for transitions where this element changes configuration.
        typedef std::pair<int, int> StateIndexPair;
        typedef std::vector<StateIndexPair> StateIndexPairs;
        StateIndexPairs stateIndexPairs01; // 0 -> 1
        StateIndexPairs stateIndexPairs10; // 1 -> 0
        
        // Get lists of (from, to) state indexes for transitions where the specified element changes configuration.
        static void getStatePairs(int elementIndex, int numStates,
                                  StateIndexPairs &stateIndexPairs01, StateIndexPairs &stateIndexPairs10);
        
        // Parse string rep of binary elements configurations for the associated state index(es).
        // "0*1" => "001, 011" => state indexes: 4, 6
        // where each set of configurations are given as reversed binary strings.
        static void getConfigurationStateIndexes(const QString &config, std::vector<int> &stateIndexes);
        
        // State names are binary string reps of the configuration of each element.
        // e.g. "011" => three elements adopting configurations 0, 1 and 1, respectively.
        static void getBinaryStateNames(int numBinaryElements, QStringList &stateNames);
        
    protected:
        // Properties.
        QString _probability0;
        QString _rate01;
        QString _rate10;
        QString _charge01;
        QString _charge10;
        int _index;
    };
    
    /* --------------------------------------------------------------------------------
     * Configuration dependent interaction between two binary elements.
     * - Factor11, FactorA1 and Factor1B are math expressions that may refer to variables
     *   and/or external stimuli by name.
     * -------------------------------------------------------------------------------- */
    class Interaction : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString A READ AName WRITE setAName)
        Q_PROPERTY(QString B READ BName WRITE setBName)
        Q_PROPERTY(QString Factor11 READ factor11 WRITE setFactor11)
        Q_PROPERTY(QString FactorA1 READ factorA1 WRITE setFactorA1)
        Q_PROPERTY(QString Factor1B READ factor1B WRITE setFactor1B)
        
    public:
        // Default constructor.
        Interaction(QObject *parent = 0, BinaryElement *A = 0, BinaryElement *B = 0) :
        QObject(parent), _A(A), _B(B), _factor11("1"), _factorA1("1"), _factor1B("1") { updateName(); }
        
        // Property getters.
        BinaryElement* A() const { return _A; }
        BinaryElement* B() const { return _B; }
        QString AName() const { return _A ? _A->name() : QString(); }
        QString BName() const { return _B ? _B->name() : QString(); }
        QString factor11() const { return _factor11; }
        QString factorA1() const { return _factorA1; }
        QString factor1B() const { return _factor1B; }
        
        // Property setters.
        void setA(BinaryElement *A) { _A = A; updateName(); }
        void setB(BinaryElement *B) { _B = B; updateName(); }
        void setAName(QString s);
        void setBName(QString s);
        void setFactor11(QString s) { _factor11 = s; }
        void setFactorA1(QString s) { _factorA1 = s; }
        void setFactor1B(QString s) { _factor1B = s; }
        
        // Lists of (from, to) state indexes for transitions afected by this interaction.
        typedef std::pair<int, int> StateIndexPair;
        typedef std::vector<StateIndexPair> StateIndexPairs;
        StateIndexPairs stateIndexPairs1101; // AB: 11 -> 01 (*= factorA1 / factor11)
        StateIndexPairs stateIndexPairs1110; // AB: 11 -> 10 (*= factor1B / factor11)
        StateIndexPairs stateIndexPairs0111; // AB: 01 -> 11 (*= factorA1)
        StateIndexPairs stateIndexPairs1011; // AB: 10 -> 11 (*= factor1B)
        
        // Get lists of (from, to) state indexes for transitions afected by an interaction between
        // specified binary elements.
        static void getStatePairs(int elementIndexA, int elementIndexB, int numStates,
                                  StateIndexPairs &stateIndexPairs1101, StateIndexPairs &stateIndexPairs1110,
                                  StateIndexPairs &stateIndexPairs0111, StateIndexPairs &stateIndexPairs1011);
        
        // Name reflects connected binary element names.
        void updateName() { setObjectName(AName() + " -- " + BName()); }
        
    protected:
        // Properties.
        BinaryElement *_A;
        BinaryElement *_B;
        QString _factor11;
        QString _factorA1;
        QString _factor1B;
    };
    
    /* --------------------------------------------------------------------------------
     * Collection of system states.
     * - States is a comma-separated list of state names or binary element configurations.
     * - Attributes is a dict-like repr of named math expressions that may each refer to
     *   variables and/or external stimuli by name.
     * -------------------------------------------------------------------------------- */
    class StateGroup : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(bool Active READ isActive WRITE setIsActive)
        Q_PROPERTY(QString States READ states WRITE setStates)
        Q_PROPERTY(QString Attributes READ attributes WRITE setAttributes)
        
    public:
        // Default constructor.
        StateGroup(QObject *parent = 0, const QString &name = "", const QString &states = "") :
        QObject(parent), _isActive(true) { setName(name); setStates(states); }
        
        // Property getters.
        QString name() const { return objectName(); }
        bool isActive() const { return _isActive; }
        QString states() const { return _states; }
        QString attributes() const { return _attributes; }
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setIsActive(bool b) { _isActive = b; }
        void setStates(QString s) { _states = s; }
        void setAttributes(QString s) { _attributes = s; }
        
        // List of state indexes belonging to this group.
        std::vector<int> stateIndexes;
        
        // Get the list of state indexes specified by a string of comma-separated state names.
        static void getStateIndexes(const QString &states, const QStringList &stateNames, std::vector<int> &stateIndexes);
        
        // Get the list of state indexes specified by a string of comma-separated binary element configurations.
        static void getStateIndexes(const QString &configs, int numBinaryElements, std::vector<int> &stateIndexes);
        
    protected:
        // Properties.
        bool _isActive;
        QString _states;
        QString _attributes;
    };
    
    /* --------------------------------------------------------------------------------
     * Markov model represented by a collection of states and transitions
     * (or binary elements and interactions).
     * -------------------------------------------------------------------------------- */
    class MarkovModel : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString Name READ name WRITE setName)
        Q_PROPERTY(QString Notes READ notes WRITE setNotes)
        
    public:
        // For dynamic object creation.
        static QObjectPropertyTreeSerializer::ObjectFactory objectFactory;
        
        // Parameters as a map of name-value pairs.
        typedef std::map<QString, double> ParameterMap;
        ParameterMap parameters; // Only valid after evalVariables() is called.
        
        // Default constructor.
        MarkovModel(QObject *parent = 0, const QString &name = "");
        
        // Property getters.
        QString name() const { return objectName(); }
        QString notes() const { return _notes; }
        
        // Property setters.
        void setName(QString s) { setObjectName(s.trimmed()); }
        void setNotes(QString s) { _notes = s; }
        
        // Find model objects.
        Transition* findTransition(State *from, State *to);
        Interaction* findInteraction(BinaryElement *A, BinaryElement *B);
        
        // Delete all model objects.
        void clear();
        
        // This must be called after altering the model structure (nodes/connections) or state groups.
        // Also supplies the names of the model's states.
        void init(QStringList &stateNames);
        
        // Evaluate each variable's value expression. This must be done prior to querying model
        // parameters such as state probabilities/attributes or transition rates/charges.
        // Input stimuli reflects external stimuli on which the model's parameters may depend.
        // Input setIndex refers to the set of variables to use, where multiple variables with the same name
        // are treated as belonging to separate variable sets. For variables with less repeats than
        // the maximum number of sets, the last repeat will be used for the remaining sets.
        void evalVariables(const ParameterMap &stimuli = ParameterMap(), size_t variableSetIndex = 0);
        size_t numVariableSets();
        
        // Get model parameters. !!! Only valid after init() and evalVariables() have been called.
        void getStateProbabilities(Eigen::VectorXd &stateProbabilities);
        void getStateAttributes(std::map<QString, Eigen::VectorXd> &stateAttributes);
        void getTransitionRates(Eigen::SparseMatrix<double> &transitionRates);
        void getTransitionCharges(Eigen::SparseMatrix<double> &transitionCharges);
        
        // Evaluate a math expression resulting in a single value.
        double evalExpr(const QString &expr);
        
        // Get/Set list of nonconstant variable values that do not depend on other parameters,
        // and also their min/max bounds. This is for parameter optimization.
        void getFreeVariables(std::vector<double> &values, std::vector<double> &min, std::vector<double> &max);
        void setFreeVariables(const std::vector<double> &values);
        
#ifdef DEBUG
        void dump(std::ostream &out = std::cout);
#endif
        
    protected:
        // Properties.
        QString _notes;
        
        // Math expression parser.
#ifdef USE_EXPR_TK
        exprtk::parser<double> _parser;
        exprtk::expression<double> _expr;
        exprtk::symbol_table<double> _symbols;
#else
        EigenLab::ParserXd _parser;
#endif
    };
    
#ifdef DEBUG
    void test();
#endif
    
} // MarkovModel

#endif
