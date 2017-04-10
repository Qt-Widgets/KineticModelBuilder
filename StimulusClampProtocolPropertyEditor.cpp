/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "StimulusClampProtocolPropertyEditor.h"
#include "StimulusClampProtocol.h"
#include <QByteArray>
#include <QHash>
#include <QLabel>
#include <QList>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>

namespace StimulusClampProtocol
{
    StimulusClampProtocolPropertyEditor::StimulusClampProtocolPropertyEditor(StimulusClampProtocol *protocol, QWidget *parent) :
    QTabWidget(parent),
    _protocol(0)
    {
        _protocolEditor = new QObjectPropertyEditor::QObjectPropertyEditor();
        _stimuliEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        
        _protocolEditor->setModel(&_protocolModel);
        _stimuliEditor->setModel(&_stimuliModel);
        
        _protocolTab = new QWidget();
        {
            _notesEditor = new QPlainTextEdit();
            connect(_notesEditor, SIGNAL(textChanged()), this, SLOT(getNotesFromEditor()));
            QVBoxLayout *layout = new QVBoxLayout(_protocolTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(_protocolEditor);
            layout->addWidget(new QLabel("Notes"));
            layout->addWidget(_notesEditor);
        }
        _stimuliTab = new QWidget();
        {
            QToolButton *addRowButton = new QToolButton();
            addRowButton->setText("+");
            connect(addRowButton, SIGNAL(clicked()), _stimuliEditor, SLOT(appendRow()));
            QVBoxLayout *layout = new QVBoxLayout(_stimuliTab);
            layout->setContentsMargins(1, 1, 1, 1);
            layout->setSpacing(1);
            layout->addWidget(addRowButton);
            layout->addWidget(_stimuliEditor);
        }
        
        addTab(_protocolTab, "Protocol");
        addTab(_stimuliTab, "Stimuli");
        
        setProtocol(protocol);
    }
    
    void StimulusClampProtocolPropertyEditor::setProtocol(StimulusClampProtocol *protocol)
    {
        if(!protocol)
            return;
        _protocol = protocol;
        
        _protocolModel.setObject(_protocol);
        _stimuliModel.setObjects(_protocol->findChildren<Stimulus*>(QString(), Qt::FindDirectChildrenOnly));
        
        QList<QByteArray> protocolPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(StimulusClampProtocol::staticMetaObject);
        QList<QByteArray> stimulusPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(Stimulus::staticMetaObject);
        
        protocolPropertyNames.removeOne("objectName");
        protocolPropertyNames.removeOne("Notes");
        stimulusPropertyNames.removeOne("objectName");
        
        QHash<QByteArray, QString> protocolPropertyHeaders;
        QHash<QByteArray, QString> stimulusPropertyHeaders;
        
        protocolPropertyHeaders["Start"] = "Start(s)";
        protocolPropertyHeaders["Duration"] = "Duration(s)";
        protocolPropertyHeaders["SampleInterval"] = "SampleInterval(s)";
        
        stimulusPropertyHeaders["Start"] = "Start(s)";
        stimulusPropertyHeaders["Duration"] = "Duration(s)";
        
        _protocolModel.setPropertyNames(protocolPropertyNames);
        _stimuliModel.setPropertyNames(stimulusPropertyNames);
        
        _protocolModel.setPropertyHeaders(protocolPropertyHeaders);
        _stimuliModel.setPropertyHeaders(stimulusPropertyHeaders);
        
        _stimuliModel.setParentOfObjects(_protocol);
        
        _stimuliModel.setObjectCreator(_stimuliModel.defaultCreator<Stimulus>);
        
        _notesEditor->setPlainText(_protocol->notes());
        
        _stimuliEditor->resizeColumnsToContents();
    }
    
    void StimulusClampProtocolPropertyEditor::getNotesFromEditor()
    {
        if(_protocol && _notesEditor)
            _protocol->setNotes(_notesEditor->toPlainText());
    }

} // StimulusClampProtocol
