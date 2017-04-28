/* --------------------------------------------------------------------------------
 * StimulusClampProtocol property editor UI.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __StimulusClampProtocolPropertyEditor_H__
#define __StimulusClampProtocolPropertyEditor_H__

#include "QObjectPropertyEditor.h"
#include <QObject>
#include <QPlainTextEdit>
#include <QTabWidget>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace StimulusClampProtocol
{
    class StimulusClampProtocol;
    
    /* --------------------------------------------------------------------------------
     * StimulusClampProtocol property editor UI.
     * -------------------------------------------------------------------------------- */
    class StimulusClampProtocolPropertyEditor : public QTabWidget
    {
        Q_OBJECT
        
    public:
        StimulusClampProtocolPropertyEditor(StimulusClampProtocol *protocol = 0, QWidget *parent = 0);
        
        // Property getters.
        StimulusClampProtocol *protocol() const { return _protocol; }
        
        // Property setters.
        void setProtocol(StimulusClampProtocol *protocol);
        
    protected:
        StimulusClampProtocol *_protocol;
        
        // Property models.
        QObjectPropertyEditor::QObjectPropertyModel _protocolModel;
        QObjectPropertyEditor::QObjectListPropertyModel _stimuliModel;
        QObjectPropertyEditor::QObjectListPropertyModel _waveformsModel;
        QObjectPropertyEditor::QObjectListPropertyModel _summariesModel;
        QObjectPropertyEditor::QObjectListPropertyModel _refDataModel;
        
        // Property model views.
        QObjectPropertyEditor::QObjectPropertyEditor *_protocolEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_stimuliEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_waveformsEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_summariesEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_refDataEditor;
        
        // UIs for each view.
        QWidget *_protocolTab;
        QWidget *_stimuliTab;
        QWidget *_waveformsTab;
        QWidget *_summariesTab;
        QWidget *_refDataTab;
        
        // Protocol notes UI.
        QPlainTextEdit *_notesEditor;
        
        QWidget* getTab(QObjectPropertyEditor::QObjectListPropertyEditor *editor);
        
    protected slots:
        void getNotesFromEditor();
    };
    
} // StimulusClampProtocol

#endif
