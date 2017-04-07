/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "MarkovModelPropertyEditor.h"
#include "MarkovModel.h"
#include <QByteArray>
#include <QHash>
#include <QLabel>
#include <QList>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>

namespace MarkovModel
{
    MarkovModelPropertyEditor::MarkovModelPropertyEditor(MarkovModel *model, QWidget *parent) :
    QTabWidget(parent),
    _model(0)
    {
        _modelEditor = new QObjectPropertyEditor::QObjectPropertyEditor();
        _variablesEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        _statesEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        _transitionsEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        _binaryElementsEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        _interactionsEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        _stateGroupsEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        
        _modelEditor->setModel(&_modelModel);
        _variablesEditor->setModel(&_variablesModel);
        _statesEditor->setModel(&_statesModel);
        _transitionsEditor->setModel(&_transitionsModel);
        _binaryElementsEditor->setModel(&_binaryElementsModel);
        _interactionsEditor->setModel(&_interactionsModel);
        _stateGroupsEditor->setModel(&_stateGroupsModel);
        
        _modelTab = new QWidget();
        {
            _notesEditor = new QPlainTextEdit();
            connect(_notesEditor, SIGNAL(textChanged()), this, SLOT(getNotesFromEditor()));
            QVBoxLayout *layout = new QVBoxLayout(_modelTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(_modelEditor);
            layout->addWidget(new QLabel("Notes"));
            layout->addWidget(_notesEditor);
        }
        _variablesTab = new QWidget();
        {
            QToolButton *addRowButton = new QToolButton();
            addRowButton->setText("+");
            connect(addRowButton, SIGNAL(clicked()), _variablesEditor, SLOT(appendRow()));
            QVBoxLayout *layout = new QVBoxLayout(_variablesTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(addRowButton);
            layout->addWidget(_variablesEditor);
        }
        _statesTab = new QWidget();
        {
            QToolButton *addRowButton = new QToolButton();
            addRowButton->setText("+");
            connect(addRowButton, SIGNAL(clicked()), _statesEditor, SLOT(appendRow()));
            QVBoxLayout *layout = new QVBoxLayout(_statesTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(addRowButton);
            layout->addWidget(_statesEditor);
        }
        _transitionsTab = new QWidget();
        {
            QToolButton *addRowButton = new QToolButton();
            addRowButton->setText("+");
            connect(addRowButton, SIGNAL(clicked()), _transitionsEditor, SLOT(appendRow()));
            QVBoxLayout *layout = new QVBoxLayout(_transitionsTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(addRowButton);
            layout->addWidget(_transitionsEditor);
        }
        _binaryElementsTab = new QWidget();
        {
            QToolButton *addRowButton = new QToolButton();
            addRowButton->setText("+");
            connect(addRowButton, SIGNAL(clicked()), _binaryElementsEditor, SLOT(appendRow()));
            QVBoxLayout *layout = new QVBoxLayout(_binaryElementsTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(addRowButton);
            layout->addWidget(_binaryElementsEditor);
        }
        _interactionsTab = new QWidget();
        {
            QToolButton *addRowButton = new QToolButton();
            addRowButton->setText("+");
            connect(addRowButton, SIGNAL(clicked()), _interactionsEditor, SLOT(appendRow()));
            QVBoxLayout *layout = new QVBoxLayout(_interactionsTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(addRowButton);
            layout->addWidget(_interactionsEditor);
        }
        _stateGroupsTab = new QWidget();
        {
            QToolButton *addRowButton = new QToolButton();
            addRowButton->setText("+");
            connect(addRowButton, SIGNAL(clicked()), _stateGroupsEditor, SLOT(appendRow()));
            QVBoxLayout *layout = new QVBoxLayout(_stateGroupsTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(addRowButton);
            layout->addWidget(_stateGroupsEditor);
        }
        
        addTab(_modelTab, "Model");
        addTab(_variablesTab, "Variables");
        addTab(_statesTab, "States");
        addTab(_transitionsTab, "Transitions");
        addTab(_binaryElementsTab, "Elements");
        addTab(_interactionsTab, "Interactions");
        addTab(_stateGroupsTab, "Groups");
        
        setModel(model);
    }
    
    MarkovModelPropertyEditor::~MarkovModelPropertyEditor()
    {
        if(indexOf(_statesTab) == -1)
            delete _statesTab;
        if(indexOf(_transitionsTab) == -1)
            delete _transitionsTab;
        if(indexOf(_binaryElementsTab) == -1)
            delete _binaryElementsTab;
        if(indexOf(_interactionsTab) == -1)
            delete _interactionsTab;
    }
    
    void MarkovModelPropertyEditor::setModel(MarkovModel *model)
    {
        if(!model)
            return;
        _model = model;
        
        _modelModel.setObject(_model);
        _variablesModel.setObjects(_model->findChildren<Variable*>(QString(), Qt::FindDirectChildrenOnly));
        _statesModel.setObjects(_model->findChildren<State*>(QString(), Qt::FindDirectChildrenOnly));
        _transitionsModel.setObjects(_model->findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly));
        _binaryElementsModel.setObjects(_model->findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly));
        _interactionsModel.setObjects(_model->findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly));
        _stateGroupsModel.setObjects(_model->findChildren<StateGroup*>(QString(), Qt::FindDirectChildrenOnly));
        
        QList<QByteArray> modelPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(MarkovModel::staticMetaObject);
        QList<QByteArray> variablePropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(Variable::staticMetaObject);
        QList<QByteArray> statePropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(State::staticMetaObject);
        QList<QByteArray> transitionPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(Transition::staticMetaObject);
        QList<QByteArray> binaryElementPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(BinaryElement::staticMetaObject);
        QList<QByteArray> interactionPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(Interaction::staticMetaObject);
        QList<QByteArray> stateGroupPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(StateGroup::staticMetaObject);
        
        modelPropertyNames.removeOne("objectName");
        modelPropertyNames.removeOne("Notes");
        variablePropertyNames.removeOne("objectName");
        statePropertyNames.removeOne("objectName");
        transitionPropertyNames.removeOne("objectName");
        binaryElementPropertyNames.removeOne("objectName");
        interactionPropertyNames.removeOne("objectName");
        stateGroupPropertyNames.removeOne("objectName");
        
        QHash<QByteArray, QString> transitionPropertyHeaders;
        QHash<QByteArray, QString> binaryElementPropertyHeaders;
        
        transitionPropertyHeaders["Rate"] = "Rate(/s)";
        transitionPropertyHeaders["Charge"] = "Charge(e)";
        
        binaryElementPropertyHeaders["Probability0"] = "0.Probability";
        binaryElementPropertyHeaders["Rate01"] = "01.Rate(/s)";
        binaryElementPropertyHeaders["Rate10"] = "10.Rate(/s)";
        binaryElementPropertyHeaders["Charge01"] = "01.Charge(e)";
        binaryElementPropertyHeaders["Charge10"] = "10.Charge(e)";
        
        _modelModel.setPropertyNames(modelPropertyNames);
        _variablesModel.setPropertyNames(variablePropertyNames);
        _statesModel.setPropertyNames(statePropertyNames);
        _transitionsModel.setPropertyNames(transitionPropertyNames);
        _binaryElementsModel.setPropertyNames(binaryElementPropertyNames);
        _interactionsModel.setPropertyNames(interactionPropertyNames);
        _stateGroupsModel.setPropertyNames(stateGroupPropertyNames);
        
        _transitionsModel.setPropertyHeaders(transitionPropertyHeaders);
        _binaryElementsModel.setPropertyHeaders(binaryElementPropertyHeaders);
        
        _variablesModel.setParentOfObjects(_model);
        _statesModel.setParentOfObjects(_model);
        _transitionsModel.setParentOfObjects(_model);
        _binaryElementsModel.setParentOfObjects(_model);
        _interactionsModel.setParentOfObjects(_model);
        _stateGroupsModel.setParentOfObjects(_model);
        
        _variablesModel.setObjectCreator(_variablesModel.defaultCreator<Variable>);
        _statesModel.setObjectCreator(_statesModel.defaultCreator<State>);
        _transitionsModel.setObjectCreator(_transitionsModel.defaultCreator<Transition>);
        _binaryElementsModel.setObjectCreator(_binaryElementsModel.defaultCreator<BinaryElement>);
        _interactionsModel.setObjectCreator(_interactionsModel.defaultCreator<Interaction>);
        _stateGroupsModel.setObjectCreator(_stateGroupsModel.defaultCreator<StateGroup>);
        
        _notesEditor->setPlainText(_model->notes());
        
        _variablesEditor->resizeColumnsToContents();
        _statesEditor->resizeColumnsToContents();
        _transitionsEditor->resizeColumnsToContents();
        _binaryElementsEditor->resizeColumnsToContents();
        _interactionsEditor->resizeColumnsToContents();
        _stateGroupsEditor->resizeColumnsToContents();
        
        bool hasBinaryElements = _model->findChild<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly) ? true : false;
        if(hasBinaryElements) {
            if(int index = indexOf(_statesTab) != -1)
                removeTab(index);
            if(int index = indexOf(_transitionsTab) != -1)
                removeTab(index);
            if(indexOf(_binaryElementsTab) == -1)
                insertTab(2, _binaryElementsTab, "Elements");
            if(indexOf(_interactionsTab) == -1)
                insertTab(3, _interactionsTab, "Interactions");
        } else {
            if(indexOf(_statesTab) == -1)
                insertTab(2, _statesTab, "States");
            if(indexOf(_transitionsTab) == -1)
                insertTab(3, _transitionsTab, "Transitions");
            if(int index = indexOf(_binaryElementsTab) != -1)
                removeTab(index);
            if(int index = indexOf(_interactionsTab) != -1)
                removeTab(index);
        }
    }
    
    void MarkovModelPropertyEditor::getNotesFromEditor()
    {
        if(_model && _notesEditor)
            _model->setNotes(_notesEditor->toPlainText());
    }

} // MarkovModel
