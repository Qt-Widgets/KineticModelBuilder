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
        _waveformsEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        _summariesEditor = new QObjectPropertyEditor::QObjectListPropertyEditor();
        
        _protocolEditor->setModel(&_protocolModel);
        _stimuliEditor->setModel(&_stimuliModel);
        _waveformsEditor->setModel(&_waveformsModel);
        _summariesEditor->setModel(&_summariesModel);
        
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
        
        addTab(_protocolTab, "Protocol");
        addTab(getTab(_stimuliEditor), "Stimuli");
        addTab(getTab(_waveformsEditor), "Waveforms");
        addTab(getTab(_summariesEditor), "Summaries");
        
        setProtocol(protocol);
    }
    
    void StimulusClampProtocolPropertyEditor::setProtocol(StimulusClampProtocol *protocol)
    {
        if(!protocol)
            return;
        _protocol = protocol;
        
        _protocolModel.setObject(_protocol);
        _stimuliModel.setObjects(_protocol->findChildren<Stimulus*>(QString(), Qt::FindDirectChildrenOnly));
        _waveformsModel.setObjects(_protocol->findChildren<Waveform*>(QString(), Qt::FindDirectChildrenOnly));
        _summariesModel.setObjects(_protocol->findChildren<SimulationsSummary*>(QString(), Qt::FindDirectChildrenOnly));
        
        QList<QByteArray> protocolPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(StimulusClampProtocol::staticMetaObject);
        QList<QByteArray> stimulusPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(Stimulus::staticMetaObject);
        QList<QByteArray> waveformPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(Waveform::staticMetaObject);
        QList<QByteArray> summaryPropertyNames = QObjectPropertyEditor::getMetaObjectPropertyNames(SimulationsSummary::staticMetaObject);
        
        protocolPropertyNames.removeOne("objectName");
        protocolPropertyNames.removeOne("Notes");
        stimulusPropertyNames.removeOne("objectName");
        waveformPropertyNames.removeOne("objectName");
        summaryPropertyNames.removeOne("objectName");
        
        QHash<QByteArray, QString> protocolPropertyHeaders;
        QHash<QByteArray, QString> stimulusPropertyHeaders;
        QHash<QByteArray, QString> summaryPropertyHeaders;
        
        protocolPropertyHeaders["Start"] = "Start(s)";
        protocolPropertyHeaders["Duration"] = "Duration(s)";
        protocolPropertyHeaders["SampleInterval"] = "SampleInterval(s)";
        
        stimulusPropertyHeaders["Start"] = "Start(s)";
        stimulusPropertyHeaders["Duration"] = "Duration(s)";
        
        summaryPropertyHeaders["StartX"] = "StartX(s)";
        summaryPropertyHeaders["DurationX"] = "DurationX(s)";
        summaryPropertyHeaders["StartY"] = "StartY(s)";
        summaryPropertyHeaders["DurationY"] = "DurationY(s)";
        
        _protocolModel.setPropertyNames(protocolPropertyNames);
        _stimuliModel.setPropertyNames(stimulusPropertyNames);
        _waveformsModel.setPropertyNames(waveformPropertyNames);
        _summariesModel.setPropertyNames(summaryPropertyNames);
        
        _protocolModel.setPropertyHeaders(protocolPropertyHeaders);
        _stimuliModel.setPropertyHeaders(stimulusPropertyHeaders);
        _summariesModel.setPropertyHeaders(summaryPropertyHeaders);
        
        _stimuliModel.setParentOfObjects(_protocol);
        _waveformsModel.setParentOfObjects(_protocol);
        _summariesModel.setParentOfObjects(_protocol);
        
        _stimuliModel.setObjectCreator(_stimuliModel.defaultCreator<Stimulus>);
        _waveformsModel.setObjectCreator(_waveformsModel.defaultCreator<Waveform>);
        _summariesModel.setObjectCreator(_summariesModel.defaultCreator<SimulationsSummary>);
        
        _notesEditor->setPlainText(_protocol->notes());
        
        _stimuliEditor->resizeColumnsToContents();
    }
    
    QWidget* StimulusClampProtocolPropertyEditor::getTab(QObjectPropertyEditor::QObjectListPropertyEditor *editor)
    {
        QToolButton *addRowButton = new QToolButton();
        addRowButton->setText("+");
        connect(addRowButton, SIGNAL(clicked()), editor, SLOT(appendRow()));
        
        QToolButton *delRowButton = new QToolButton();
        delRowButton->setText("-");
        connect(delRowButton, SIGNAL(clicked()), editor, SLOT(removeSelectedRows()));
        
        QHBoxLayout *buttons = new QHBoxLayout();
        buttons->setContentsMargins(1, 1, 1, 1);
        buttons->setSpacing(1);
        buttons->addWidget(addRowButton);
        buttons->addWidget(delRowButton);
        buttons->addStretch();
        
        QWidget *tab = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(tab);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addLayout(buttons);
        layout->addWidget(editor);
        return tab;
    }
    
    void StimulusClampProtocolPropertyEditor::getNotesFromEditor()
    {
        if(_protocol && _notesEditor)
            _protocol->setNotes(_notesEditor->toPlainText());
    }

} // StimulusClampProtocol
