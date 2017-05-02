/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "StimulusClampProtocolPlot.h"
#include "QObjectPropertyEditor.h"
#include "StimulusClampProtocol.h"
#include "StimulusClampProtocolWindow.h"
#include <algorithm>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_engine.h>
#include <qwt_symbol.h>
#include <QDialog>
#include <QFileDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWidgetAction>

namespace StimulusClampProtocol
{
    StimulusClampProtocolPlotPicker::StimulusClampProtocolPlotPicker(QwtPlotCanvas *canvas) :
    QwtPlotPicker(canvas)
    {
        setStateMachine(new QwtPickerClickPointMachine());
        setTrackerMode(QwtPicker::AlwaysOn);
    }
    
    QwtPlotCurve* StimulusClampProtocolPlotPicker::closestCurve(const QPointF &pos, double withinPixels) const
    {
        QPoint pixelPos = transform(pos);
        double dist, mindist = -1;
        int itemIndex = -1;
        QwtPlotItemList items = plot()->itemList();
        for(int i = 0; i < items.size(); ++i) {
            if(items[i]->rtti() == QwtPlotItem::Rtti_PlotCurve) {
                if(((QwtPlotCurve *) items[i])->closestPoint(pixelPos, & dist) >= 0) {
                    if((dist <= withinPixels || withinPixels == 0) && (mindist == -1 || dist < mindist)) {
                        mindist = dist;
                        itemIndex = i;
                    }
                }
            }
        }
        return (itemIndex != -1 ? (QwtPlotCurve*)(items[itemIndex]) : 0);
    }
    
    void StimulusClampProtocolPlotPicker::selectCurve(const QPointF &pos)
    {
        QwtPlotCurve *selectedCurve = closestCurve(pos, 10);
        if(selectedCurve)
            emit curveSelected(selectedCurve);
    }
    
    QwtText StimulusClampProtocolPlotPicker::trackerTextF(const QPointF &pos) const
    {
        if(QwtPlotCurve *selectedCurve = closestCurve(pos, 10))
            return selectedCurve->title();
        return QwtText(QString(""));
    }
    
    StimulusClampProtocolPlot::StimulusClampProtocolPlot(QWidget *parent) :
    QwtPlot(parent),
    _protocol(0),
    _visibleEventChains("0"),
    _showEventChains(false),
    _showReferenceData(true),
    _isLogScaleX(false),
    _isLogScaleYLeft(false),
    _isLogScaleYRight(false),
    _lineWidth(2),
    _markerSize(0),
    _referenceDataColor(128, 128, 128),
    _picker(qobject_cast<QwtPlotCanvas*>(canvas())),
    _zoomer(canvas())
    {
        connect(this, SIGNAL(optionsChanged()), this, SLOT(plotProtocol()));
        
        // Default color map.
        _colorMap.reserve(7);
        _colorMap.push_back(QColor(  0, 114, 189));
        _colorMap.push_back(QColor(217,  83,  25));
        _colorMap.push_back(QColor(237, 177,  32));
        _colorMap.push_back(QColor(126,  47, 142));
        _colorMap.push_back(QColor(119, 172,  48));
        _colorMap.push_back(QColor( 77, 190, 238));
        _colorMap.push_back(QColor(162,  20,  47));
    }
    
    QMenu* StimulusClampProtocolPlot::getMenu()
    {
        
        QWidget *ui = new QWidget();
        QLineEdit *visLeftEdit = new QLineEdit(visibleSignalsYLeft());
        QLineEdit *visRightEdit = new QLineEdit(visibleSignalsYRight());
        connect(visLeftEdit, SIGNAL(textEdited(QString)), this, SLOT(_setVisibleSignalsYLeft(QString)));
        connect(visRightEdit, SIGNAL(textEdited(QString)), this, SLOT(_setVisibleSignalsYRight(QString)));
        QGridLayout *grid = new QGridLayout(ui);
        grid->setContentsMargins(5, 1, 5, 1);
        grid->setSpacing(1);
        grid->addWidget(new QLabel("Y Axis Left"), 0, 0);
        grid->addWidget(visLeftEdit, 0, 1);
        grid->addWidget(new QLabel("Y Axis Right"), 1, 0);
        grid->addWidget(visRightEdit, 1, 1);
        QWidgetAction *uiAction = new QWidgetAction(0);
        uiAction->setDefaultWidget(ui);
        
        QMenu *menu = new QMenu();
        menu->addAction(uiAction);
        menu->addSeparator();
        menu->addAction("Plot Options", this, SLOT(editOptions()));
        menu->addSeparator();
        menu->addAction("Export Visible (.txt)", this, SLOT(exportVisibleToText()));
        menu->addAction("Export Visible (.svg)", this, SLOT(exportVisibleToSvg()));
        menu->addAction("Export Monte Carlo Event Chains (.dwt)", this, SLOT(exportMonteCarloEventChainsToDwt()));
        return menu;
    }
    
    void StimulusClampProtocolPlot::clearPlot()
    {
        detachItems(); // Also deletes the curves.
        _curves.clear();
        _curveTitlesXY.clear();
        enableAxis(QwtPlot::yLeft, true);
        enableAxis(QwtPlot::yRight, !visibleSignalsYRight().isEmpty());
    }
    
    void StimulusClampProtocolPlot::plotProtocol()
    {
        clearPlot();
        if(!_protocol) return;
        int rows = _protocol->simulations.size();
        int cols = rows ? _protocol->simulations[0].size() : 0;
        int sets = cols ? _protocol->simulations[0][0].waveforms.size() : 0;
        if(sets == 0 || rows == 0 || cols == 0)
            return;
        std::vector<std::string> visLeft = str2vec<std::string>(visibleSignalsYLeft());
        std::vector<std::string> visRight = str2vec<std::string>(visibleSignalsYRight());
        std::vector<size_t> visVarSets = str2vec<size_t>(visibleVariableSetIndexes());
        std::vector<size_t> visRows = str2vec<size_t>(visibleRows());
        std::vector<size_t> visCols = str2vec<size_t>(visibleColumns());
        std::vector<size_t> visChains = str2vec<size_t>(visibleEventChains());
        std::sort(visVarSets.begin(), visVarSets.end()); // ascending order
        std::sort(visRows.begin(), visRows.end()); // ascending order
        std::sort(visCols.begin(), visCols.end()); // ascending order
        std::sort(visChains.begin(), visChains.end()); // ascending order
        visVarSets.resize(std::distance(visVarSets.begin(), std::unique(visVarSets.begin(), visVarSets.end()))); // keep unique only
        visRows.resize(std::distance(visRows.begin(), std::unique(visRows.begin(), visRows.end()))); // keep unique only
        visCols.resize(std::distance(visCols.begin(), std::unique(visCols.begin(), visCols.end()))); // keep unique only
        visChains.resize(std::distance(visChains.begin(), std::unique(visChains.begin(), visChains.end()))); // keep unique only
        if(visVarSets.empty() && sets) {
            visVarSets.resize(sets);
            std::iota(visVarSets.begin(), visVarSets.end(), 0); // indexes = {0, 1, 2, ...}
        }
        if(visRows.empty() && rows) {
            visRows.resize(rows);
            std::iota(visRows.begin(), visRows.end(), 0); // indexes = {0, 1, 2, ...}
        }
        if(visCols.empty() && cols) {
            visCols.resize(cols);
            std::iota(visCols.begin(), visCols.end(), 0); // indexes = {0, 1, 2, ...}
        }
//        qDebug() << "visVarSets: " << QVector<size_t>::fromStdVector(visVarSets);
//        qDebug() << "visRows: " << QVector<size_t>::fromStdVector(visRows);
//        qDebug() << "visCols: " << QVector<size_t>::fromStdVector(visCols);
        QList<SimulationsSummary*> summaries = _protocol->findChildren<SimulationsSummary*>(QString(), Qt::FindDirectChildrenOnly);
        int colorIndex = 0;
        for(int yAxis : {QwtPlot::yLeft, QwtPlot::yRight}) {
            std::vector<std::string> &visSignals = yAxis == QwtPlot::yLeft ? visLeft : visRight;
            if(yAxis == QwtPlot::yRight && visSignals.empty()) {
                enableAxis(QwtPlot::yRight, false);
                break;
            } else {
                enableAxis(QwtPlot::yRight, true);
            }
            for(size_t varSet : visVarSets) {
                for(size_t row : visRows) {
                    if(row < _protocol->simulations.size()) {
                        QString rowPostfix = " (";
                        if(sets > 1) rowPostfix += QString::number(varSet) + ",";
                        if(rows > 1) rowPostfix += QString::number(row) + ",";
                        if(cols > 1) rowPostfix += ":";
                        if(rowPostfix.endsWith(","))
                            rowPostfix.chop(1);
                        rowPostfix += ")";
                        if(rowPostfix == " ()")
                            rowPostfix = "";
                        for(size_t col : visCols) {
                            if(col < _protocol->simulations[row].size()) {
                                Simulation &sim = _protocol->simulations[row][col];
                                QString postfix = rowPostfix;
                                if(cols > 1) {
                                    postfix.chop(2);
                                    postfix += QString::number(col) + ")";
                                }
                                if(yAxis == QwtPlot::yLeft && _showEventChains) {
                                    if(sim.events.size() > varSet) {
                                        const std::vector<MonteCarloEventChain> &eventChains = sim.events.at(varSet);
                                        std::vector<size_t> chainIndexes = visChains;
                                        if(chainIndexes.empty()) {
                                            chainIndexes.resize(eventChains.size());
                                            std::iota(chainIndexes.begin(), chainIndexes.end(), 0); // indexes = {0, 1, 2, ...}
                                        }
                                        std::vector<double> eventTimes;
                                        std::vector<double> eventStates;
                                        for(size_t visChain : chainIndexes) {
                                            if(visChain < eventChains.size()) {
                                                const MonteCarloEventChain &eventChain = eventChains.at(visChain);
                                                eventTimes.resize(eventChain.size() + 1);
                                                eventStates.resize(eventChain.size() + 1);
                                                double cumTime = 0;
                                                size_t eventCounter = 0;
                                                for(const MonteCarloEvent &event : eventChain) {
                                                    eventTimes[eventCounter] = cumTime;
                                                    eventStates[eventCounter] = event.state;
                                                    cumTime += event.duration;
                                                    ++eventCounter;
                                                }
                                                eventTimes.back() = cumTime;
                                                eventStates.back() = eventStates[eventCounter - 1];
                                                QString chainsPostfix = postfix;
                                                if(chainsPostfix.size()) {
                                                    chainsPostfix.chop(1);
                                                    chainsPostfix += "," + QString::number(visChain) + ")";
                                                } else {
                                                    chainsPostfix = " (" + QString::number(visChain) + ")";
                                                }
                                                addCurve(yAxis, "Time (s)", "State", chainsPostfix, eventTimes.data(), eventStates.data(), eventTimes.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Steps, false);
                                            }
                                        } // visChain
                                    }
                                } else if(visSignals.empty()) {
                                    // Plot probability in each state.
                                    if(sim.probability.size() > varSet) {
                                        Eigen::MatrixXd &probability = sim.probability.at(varSet);
                                        for(int i = 0; i < probability.cols(); ++i) {
                                            QString stateName = _protocol->stateNames.at(i);
                                            if(_showReferenceData && sim.referenceData.size() > varSet) {
                                                std::map<QString, Simulation::RefData>::iterator refIter = sim.referenceData.at(varSet).find(stateName);
                                                if(refIter != sim.referenceData.at(varSet).end()) {
                                                    Simulation::RefData &refData = refIter->second;
                                                    addCurve(yAxis, "Time (s)", "Ref " + stateName, postfix, sim.time.data() + refData.firstPt, refData.waveform.data(), refData.numPts, _referenceDataColor, QwtPlotCurve::Lines);
                                                }
                                            }
                                            addCurve(yAxis, "Time (s)", stateName, postfix, sim.time.data(), probability.col(i).data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                        }
                                    }
                                    setAxisTitle(QwtPlot::xBottom, "Time (s)");
                                    setAxisTitle(yAxis, "Probability");
                                } else {
                                    // Plot listed signals.
                                    for(const std::string &visSignal : visSignals) {
                                        QString visSig = QString::fromStdString(visSignal);
                                        if(visSig.toLower() == "weight") {
                                            addCurve(yAxis, "Time (s)", "Weight", postfix, sim.time.data(), sim.weight.data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                        } else if(visSig.toLower() == "mask") {
                                            Eigen::VectorXd mask = sim.mask.cast<double>();
                                            addCurve(yAxis, "Time (s)", "Mask", postfix, sim.time.data(), mask.data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                        } else {
                                            double *x = 0;
                                            double *y = 0;
                                            int n = 0;
                                            _protocol->getSimulationWaveform(visSig, sim, varSet, &x, &y, &n);
                                            if(x && y && n > 0) {
                                                if(_showReferenceData) {
                                                    double *xref = 0;
                                                    double *yref = 0;
                                                    int nref = 0;
                                                    _protocol->getSimulationRefWaveform(visSig, sim, varSet, &xref, &yref, &nref);
                                                    if(xref && yref && nref > 0) {
                                                        addCurve(yAxis, "Time (s)", "Ref " + visSig, postfix, xref, yref, nref, _referenceDataColor, QwtPlotCurve::Lines);
                                                    }
                                                }
                                                addCurve(yAxis, "Time (s)", visSig, postfix, x, y, n, _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                            } else if(col == visCols.at(0)) {
                                                QString xTitle, yTitle;
                                                _protocol->getSummaryWaveform(visSig, varSet, row, &x, &y, &n, &xTitle, &yTitle);
                                                if(x && y && n > 0) {
                                                    if(_showReferenceData) {
                                                        double *xref = 0;
                                                        double *yref = 0;
                                                        int nref = 0;
                                                        _protocol->getSummaryRefWaveform(visSig, varSet, row, &xref, &yref, &nref);
                                                        if(xref && yref && nref > 0) {
                                                            addCurve(yAxis, xTitle, "Ref " + visSig, postfix, xref, yref, nref, _referenceDataColor, QwtPlotCurve::Lines);
                                                        }
                                                    }
                                                    addCurve(yAxis, xTitle, visSig, postfix, x, y, n, _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                                }
                                            }
                                        }
                                    } // visSignal
                                }
                            }
                        } // col
                    }
                } // row
            } // varSet
        } // yAxis
        if(isLogScaleX())
            setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
        else
            setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());
        if(isLogScaleYLeft())
            setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
        else
            setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
        if(axisEnabled(QwtPlot::yRight)) {
            if(isLogScaleYRight())
                setAxisScaleEngine(QwtPlot::yRight, new QwtLogScaleEngine());
            else
                setAxisScaleEngine(QwtPlot::yRight, new QwtLinearScaleEngine());
        }
        autoscale();
    }
    
    void StimulusClampProtocolPlot::autoscale()
    {
        setAxisAutoScale(_zoomer.xAxis());
        setAxisAutoScale(_zoomer.yAxis());
        _zoomer.setZoomBase();
        replot();
    }
    
    void StimulusClampProtocolPlot::editOptions()
    {
        QObjectPropertyEditor::QObjectPropertyModel propertyModel;
        propertyModel.setObject(this);
        
        QList<QByteArray> propertyNames = QObjectPropertyEditor::getObjectPropertyNames(this);
        QwtPlot widget;
        QList<QByteArray> widgetPropertyNames = QObjectPropertyEditor::getObjectPropertyNames(&widget);
        foreach(const QByteArray &propertyName, widgetPropertyNames)
            propertyNames.removeOne(propertyName);
        if(parent() && parent()->parent() && qobject_cast<StimulusClampProtocolWindow*>(parent()->parent())) {
            propertyNames.removeOne("VisibleVariableSets");
            propertyNames.removeOne("VisibleRows");
            propertyNames.removeOne("VisibleColumns");
            propertyNames.removeOne("VisibleEventChains");
        }
        propertyModel.setPropertyNames(propertyNames);
        
        QObjectPropertyEditor::QObjectPropertyEditor *editor = new QObjectPropertyEditor::QObjectPropertyEditor();
        editor->setModel(&propertyModel);
        
        QDialog dialog(this);
        dialog.setModal(true);
        dialog.setWindowTitle("Plot Options");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(editor);
        dialog.show();
        editor->resizeColumnToContents(0);
        if(editor->columnWidth(0) < 200)
            editor->setColumnWidth(0, 200);
        QPoint topLeft = mapToGlobal(QPoint(0, 0));
        dialog.setGeometry(topLeft.x(), topLeft.y(), 400, 300);
        dialog.exec();
    }

    void StimulusClampProtocolPlot::exportVisibleToText()
    {
        QString filePath = QFileDialog::getSaveFileName(this, "Export visible plots to column data *.txt file.");
        if(filePath.size() == 0)
            return;
        if(_curves.size() == 0) {
            QString messageTitle = "Export warning.";
            QString message = "No simulations are displayed on this plot. Only displayed simulations are exported.\r\n";
            QMessageBox::information(this, messageTitle, message);
            return;
        }
        QFile file(filePath);
        if(!file.open(QIODevice::Text | QIODevice::WriteOnly))
            return;
        QTextStream out(&file);
        // Curves of the same size.
        std::vector<size_t> curveIndexes;
        curveIndexes.reserve(_curves.size());
        curveIndexes.push_back(0);
        size_t numPts = _curves[0]->dataSize();
        for(size_t i = 1; i < _curves.size(); ++i) {
            if(_curves[i]->dataSize() == numPts)
                curveIndexes.push_back(i);
        }
        // Titles.
        for(size_t i : curveIndexes)
            out << (i == curveIndexes[0] ? "" : "\t") << _curveTitlesXY[i].first << "\t" << _curveTitlesXY[i].second;
        out << "\r\n";
        // Data.
        for(size_t row = 0; row < numPts; ++row) {
            for(size_t i : curveIndexes){
                QwtPlotCurve *curve = _curves[i];
                out << (i == curveIndexes[0] ? "" : "\t") << curve->sample(row).x() << "\t" << curve->sample(row).y();
            }
            out << "\r\n";
        }
        file.close();
        // If not everything was exported due to different length arrays.
        if(curveIndexes.size() != _curves.size()) {
            QString messageTitle = "Export warning.";
            QString message = QString::number(curveIndexes.size()) + " of " + QString::number(_curves.size()) + " visible plots exported.\r\n";
            message = message + "Remaining plots not exported as they contain a different number of sample points.\r\n";
            message = message + "To export all plots, only display plots with equal sample sizes.\r\n";
            QMessageBox::information(this, messageTitle, message);
        }
    }

    void StimulusClampProtocolPlot::exportVisibleToSvg()
    {
        QString filePath = QFileDialog::getSaveFileName(this, "Export visible plots to *.svg file.");
        if(!filePath.isEmpty()) {
            QwtPlotRenderer renderer;
            replot();
            renderer.renderDocument(this, filePath, "svg", QSizeF(300, 300));
        }
    }

    void StimulusClampProtocolPlot::exportMonteCarloEventChainsToDwt()
    {
        if(_protocol)
            _protocol->saveMonteCarloEventChainsAsDwt();
    }
    
    QwtPlotCurve* StimulusClampProtocolPlot::addCurve(int yAxis, const QString &xTitle, const QString &yTitle, const QString &yPostFix, double *x, double *y, int npts, const QColor &color, QwtPlotCurve::CurveStyle style, bool isRawData)
    {
        QwtPlotCurve *curve = new QwtPlotCurve;
        if(lineWidth() > 0) {
            curve->setStyle(style);
            if(style == QwtPlotCurve::Steps)
                curve->setCurveAttribute(QwtPlotCurve::Inverted, true);
        } else {
            curve->setStyle(QwtPlotCurve::NoCurve);
        }
        if(markerSize() > 0) {
            QwtSymbol *marker = new QwtSymbol(QwtSymbol::Ellipse);
            marker->setPen(QPen(color));
            marker->setSize(markerSize());
            curve->setSymbol(marker);
        }
        curve->setPen(QPen(color, lineWidth()));
        curve->setTitle(yTitle + yPostFix);
        if(isRawData)
            curve->setRawSamples(x, y, npts);
        else
            curve->setSamples(x, y, npts);
        curve->setAxes(QwtPlot::xBottom, yAxis);
        curve->attach(this);
        if(yAxis == QwtPlot::yRight) {
            enableAxis(QwtPlot::yRight, true);
            curve->setZ(curve->z() - 10000.); // yRight behind yLeft.
        }
        _curves.push_back(curve);
        _curveTitlesXY.push_back(std::pair<QString, QString>(xTitle, yTitle + yPostFix));
        setAxisTitle(QwtPlot::xBottom, xTitle);
        setAxisTitle(yAxis, yTitle);
        return curve;
    }
    
    void StimulusClampProtocolPlot::mouseReleaseEvent(QMouseEvent *event)
    {
        if(event->button() == Qt::RightButton && !canvas()->geometry().contains(event->x(), event->y()))
            getMenu()->exec(QCursor::pos());
        else
            QwtPlot::mouseReleaseEvent(event);
    }

} // StimulusClampProtocol
