/* --------------------------------------------------------------------------------
 * 3D viewer for a MarkovModel.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __StimulusClampProtocolWindow_H__
#define __StimulusClampProtocolWindow_H__

#include "QMultipleIndexSpinBox.h"
#include <vector>
#include <QAction>
#include <QGridLayout>
#include <QLineEdit>
#include <QMainWindow>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif
namespace KineticModelBuilder { class Project; }

namespace StimulusClampProtocol
{
    class StimulusClampProtocol;
    class StimulusClampProtocolPlot;
    
    /* --------------------------------------------------------------------------------
     * UI window for a MarkovModel.
     * -------------------------------------------------------------------------------- */
    class StimulusClampProtocolWindow : public QMainWindow
    {
        Q_OBJECT
        
    public:
        StimulusClampProtocolWindow(StimulusClampProtocol* protocol, QWidget *parent = 0);
        
        StimulusClampProtocol* protocol() const { return _protocol; }
        
        void resizePlotGrid(int rows, int cols);
        int plotRows() const { return _plotRows; }
        int plotColumns() const { return _plotColumns; }
        
    public slots:
        void replot();
        void setPlotRows(int rows) { resizePlotGrid(rows, _plotColumns); }
        void setPlotColumns(int cols) { resizePlotGrid(_plotRows, cols); }
        void editProtocol();
        
    signals:
        void plotGridResized();
        
    protected:
        KineticModelBuilder::Project *_project;
        StimulusClampProtocol *_protocol;
        QGridLayout *_plotGrid;
        std::vector<StimulusClampProtocolPlot*> _plots;
        int _plotRows;
        int _plotColumns;
        QMultipleIndexSpinBox *_visibleVariableSetsBox;
        QMultipleIndexSpinBox *_visibleRowsBox;
        QMultipleIndexSpinBox *_visibleColumnsBox;
        QMultipleIndexSpinBox *_visibleEventChainsBox;
        QAction *_visibleVariableSetsBoxAction;
        QAction *_visibleRowsBoxAction;
        QAction *_visibleColumnsBoxAction;
        QAction *_visibleEventChainsBoxAction;
        
    protected slots:
        void checkIfWeNeedToShowTheEventChainUi();
    };
    
} // StimulusClampProtocol

#endif
