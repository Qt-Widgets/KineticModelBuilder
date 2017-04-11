/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "StimulusClampProtocolPlot.h"
#include "QObjectPropertyEditor.h"
#include "StimulusClampProtocol.h"
#include <algorithm>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_engine.h>
#include <qwt_symbol.h>
//#include <QApplication>
#include <QDialog>
#include <QFileDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QVBoxLayout>
//#include <QVector>

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
    
    void StimulusClampProtocolPlot::clearPlot()
    {
        detachItems(); // Also deletes the curves.
        _curves.clear();
        _curveTitlesXY.clear();
        enableAxis(QwtPlot::yLeft, true);
        enableAxis(QwtPlot::yRight, !visibleSignalsYRight().isEmpty());
    }
    
    void StimulusClampProtocolPlot::plotProtocol(StimulusClampProtocol *protocol, const QStringList &stateNames)
    {
        std::vector<std::string> visLeft = str2vec<std::string>(visibleSignalsYLeft());
        std::vector<std::string> visRight = str2vec<std::string>(visibleSignalsYRight());
        std::vector<size_t> visVarSets = str2vec<size_t>(visibleVariableSetIndexes());
        std::vector<size_t> visRows = str2vec<size_t>(visibleRows());
        std::vector<size_t> visCols = str2vec<size_t>(visibleColumns());
        std::sort(visVarSets.begin(), visVarSets.end()); // ascending order
        std::sort(visRows.begin(), visRows.end()); // ascending order
        std::sort(visCols.begin(), visCols.end()); // ascending order
        visVarSets.resize(std::distance(visVarSets.begin(), std::unique(visVarSets.begin(), visVarSets.end()))); // keep unique only
        visRows.resize(std::distance(visRows.begin(), std::unique(visRows.begin(), visRows.end()))); // keep unique only
        visCols.resize(std::distance(visCols.begin(), std::unique(visCols.begin(), visCols.end()))); // keep unique only
        if(visVarSets.empty()) {
            visVarSets.resize(protocol->simulations[0][0].probability.size());
            std::iota(visVarSets.begin(), visVarSets.end(), 0); // indexes = {0, 1, 2, ...}
        }
        if(visRows.empty()) {
            visRows.resize(protocol->simulations.size());
            std::iota(visRows.begin(), visRows.end(), 0); // indexes = {0, 1, 2, ...}
        }
        if(visCols.empty()) {
            visCols.resize(protocol->simulations[0].size());
            std::iota(visCols.begin(), visCols.end(), 0); // indexes = {0, 1, 2, ...}
        }
//        qDebug() << "visVarSets: " << QVector<size_t>::fromStdVector(visVarSets);
//        qDebug() << "visRows: " << QVector<size_t>::fromStdVector(visRows);
//        qDebug() << "visCols: " << QVector<size_t>::fromStdVector(visCols);
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
                    if(row < protocol->simulations.size()) {
                        for(size_t col : visCols) {
                            if(col < protocol->simulations[row].size()) {
                                Simulation &sim = protocol->simulations[row][col];
                                bool hasProbability = sim.probability.size() > varSet;
                                bool hasWaveforms = sim.waveforms.size() > varSet;
                                QString postfix = " (" + QString::number(varSet) + "," + QString::number(row) + "," + QString::number(col) + ")";
                                if(visSignals.empty()) {
                                    // Plot probability in each state.
                                    if(hasProbability) {
                                        Eigen::MatrixXd &probability = sim.probability.at(varSet);
                                        for(int i = 0; i < probability.cols(); ++i) {
                                            addCurve(yAxis, "Time (s)", stateNames.at(i) + postfix, sim.time.data(), probability.col(i).data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                        }
                                    }
                                } else {
                                    for(const std::string &visSignal : visSignals) {
                                        QString visSig = QString::fromStdString(visSignal);
                                        if(visSig.toLower() == "weight") {
                                            addCurve(yAxis, "Time (s)", "weight" + postfix, sim.time.data(), sim.weight.data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                        } else if(visSig.toLower() == "mask") {
                                            Eigen::VectorXd mask = sim.mask.cast<double>();
                                            addCurve(yAxis, "Time (s)", "mask" + postfix, sim.time.data(), mask.data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                        } else {
                                            int i = stateNames.indexOf(visSig);
                                            if(i != -1 && hasProbability) {
                                                Eigen::MatrixXd &probability = sim.probability.at(varSet);
                                                addCurve(yAxis, "Time (s)", visSig + postfix, sim.time.data(), probability.col(i).data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                            } else {
                                                auto it = sim.stimuli.find(visSig);
                                                if(it != sim.stimuli.end()) {
                                                    addCurve(yAxis, "Time (s)", visSig + postfix, sim.time.data(), it->second.data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                                } else if(hasWaveforms) {
                                                    std::map<QString, Eigen::VectorXd> &waveforms = sim.waveforms.at(varSet);
                                                    it = waveforms.find(visSig);
                                                    if(it != waveforms.end()) {
                                                        addCurve(yAxis, "Time (s)", visSig + postfix, sim.time.data(), it->second.data(), sim.time.size(), _colorMap.at(colorIndex++ % _colorMap.size()), QwtPlotCurve::Lines);
                                                    }
                                                }
                                            }
                                        }
                                    }
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
        replot();
    }
    
    QwtPlotCurve* StimulusClampProtocolPlot::addCurve(int yAxis, const QString &xTitle, const QString &yTitle, double *x, double *y, int npts, const QColor &color, QwtPlotCurve::CurveStyle style)
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
        curve->setTitle(yTitle);
        curve->setRawSamples(x, y, npts);
        curve->setAxes(QwtPlot::xBottom, yAxis);
        curve->attach(this);
        if(yAxis == QwtPlot::yRight) {
            enableAxis(QwtPlot::yRight, true);
            curve->setZ(curve->z() - 10000.); // yRight behind yLeft.
        }
        _curves.push_back(curve);
        _curveTitlesXY.push_back(std::pair<QString, QString>(xTitle, yTitle));
        return curve;
    }
    
    QMenu* StimulusClampProtocolPlot::getMenu()
    {
        QMenu *menu = new QMenu();
        menu->addAction("Plot Options", this, SLOT(editOptions()));
        menu->addSeparator();
        menu->addAction("Export Visible (.txt)", this, SLOT(exportVisibleToText()));
        menu->addAction("Export Visible (.svg)", this, SLOT(exportVisibleToSvg()));
        menu->addAction("Export Monte Carlo Event Chains (.dwt)", this, SLOT(exportMonteCarloEventChainsToDwt()));
        return menu;
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
        propertyModel.setPropertyNames(propertyNames);
        
        QObjectPropertyEditor::QObjectPropertyEditor *editor = new QObjectPropertyEditor::QObjectPropertyEditor();
        editor->setModel(&propertyModel);
        
        QDialog dialog(this);
        dialog.setWindowTitle("Plot Options");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(editor);
        dialog.show();
        editor->resizeColumnToContents(0);
        if(editor->columnWidth(0) < 200)
            editor->setColumnWidth(0, 200);
//        qApp->processEvents();
//        dialog.resize(editor->width(), editor->height());
        QPoint topLeft = mapToGlobal(QPoint(0, 0));
        dialog.setGeometry(topLeft.x(), topLeft.y(), 400, 300);
        dialog.exec();
    }

    void StimulusClampProtocolPlot::exportVisibleToText()
    {
        // ... TODO
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
        // ... TODO
    }
    
    void StimulusClampProtocolPlot::mouseReleaseEvent(QMouseEvent *event)
    {
        if(event->button() == Qt::RightButton && !canvas()->geometry().contains(event->x(), event->y()))
            getMenu()->exec(QCursor::pos());
        else
            QwtPlot::mouseReleaseEvent(event);
    }

} // StimulusClampProtocol
