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
        
        // Property model views.
        QObjectPropertyEditor::QObjectPropertyEditor *_protocolEditor;
        QObjectPropertyEditor::QObjectListPropertyEditor *_stimuliEditor;
        
        // UIs for each view.
        QWidget *_protocolTab;
        QWidget *_stimuliTab;
        
        // Protocol notes UI.
        QPlainTextEdit *_notesEditor;
        
    protected slots:
        void getNotesFromEditor();
    };
    
} // StimulusClampProtocol

#endif
