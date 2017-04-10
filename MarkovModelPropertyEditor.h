/* --------------------------------------------------------------------------------
 * MarkovModel property editor UI.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __MarkovModelPropertyEditor_H__
#define __MarkovModelPropertyEditor_H__

#include "QObjectPropertyEditor.h"
#include <QObject>
#include <QPlainTextEdit>
#include <QTabWidget>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace MarkovModel
{
    class MarkovModel;
    
    /* --------------------------------------------------------------------------------
     * MarkovModel property editor UI.
     * -------------------------------------------------------------------------------- */
    class MarkovModelPropertyEditor : public QTabWidget
    {
        Q_OBJECT
        
    public:
        MarkovModelPropertyEditor(MarkovModel *model = 0, QWidget *parent = 0);
        ~MarkovModelPropertyEditor();
        
        // Property getters.
        MarkovModel *model() const { return _model; }
        
        // Property setters.
        void setModel(MarkovModel *model);
        
    protected:
        MarkovModel *_model;
        
        // Property models.
        QObjectPropertyEditor::QObjectPropertyModel _modelModel;
        QObjectPropertyEditor::QObjectListPropertyModel _variablesModel;
        QObjectPropertyEditor::QObjectListPropertyModel _statesModel;
        QObjectPropertyEditor::QObjectListPropertyModel _transitionsModel;
        QObjectPropertyEditor::QObjectListPropertyModel _binaryElementsModel;
        QObjectPropertyEditor::QObjectListPropertyModel _interactionsModel;
        QObjectPropertyEditor::QObjectListPropertyModel _stateGroupsModel;
        
        // Property model views.
        QObjectPropertyEditor::QObjectPropertyEditor *_modelEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_variablesEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_statesEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_transitionsEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_binaryElementsEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_interactionsEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_stateGroupsEditor;
        
        // UIs for each view.
        QWidget *_modelTab;
        QWidget *_variablesTab;
        QWidget *_statesTab;
        QWidget *_transitionsTab;
        QWidget *_binaryElementsTab;
        QWidget *_interactionsTab;
        QWidget *_stateGroupsTab;
        
        // Model notes UI.
        QPlainTextEdit *_notesEditor;
        
    protected slots:
        void getNotesFromEditor();
    };
    
} // MarkovModel

#endif
