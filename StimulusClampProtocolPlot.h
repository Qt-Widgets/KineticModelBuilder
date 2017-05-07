/* --------------------------------------------------------------------------------
 *
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __StimulusClampProtocolPlot_H__
#define __StimulusClampProtocolPlot_H__

#include <vector>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif
class QMenu;

namespace StimulusClampProtocol
{
    class StimulusClampProtocol;
    
    /* --------------------------------------------------------------------------------
     * Plot picker that displays the name of a curve when the mouse hovers over it.
     * -------------------------------------------------------------------------------- */
    class StimulusClampProtocolPlotPicker : public QwtPlotPicker
    {
        Q_OBJECT
    public:
        StimulusClampProtocolPlotPicker(QwtPlotCanvas *canvas);
        
        public slots:
        QwtPlotCurve* closestCurve(const QPointF &pos, double withinPixels = 0) const;
        void selectCurve(const QPointF &pos);
        
    signals:
        void curveSelected(QwtPlotCurve*);
        
    protected:
        QwtText trackerTextF(const QPointF &pos) const;
    };
    
    /* --------------------------------------------------------------------------------
     * -------------------------------------------------------------------------------- */
    class StimulusClampProtocolPlot : public QwtPlot
    {
        Q_OBJECT
        Q_PROPERTY(QString VisibleYLeft READ visibleSignalsYLeft WRITE setVisibleSignalsYLeft NOTIFY optionsChanged)
        Q_PROPERTY(QString VisibleYRight READ visibleSignalsYRight WRITE setVisibleSignalsYRight NOTIFY optionsChanged)
        Q_PROPERTY(QString VisibleVariableSets READ visibleVariableSetIndexes WRITE setVisibleVariableSetIndexes NOTIFY optionsChanged)
        Q_PROPERTY(QString VisibleRows READ visibleRows WRITE setVisibleRows NOTIFY optionsChanged)
        Q_PROPERTY(QString VisibleColumns READ visibleColumns WRITE setVisibleColumns NOTIFY optionsChanged)
        Q_PROPERTY(QString VisibleEventChains READ visibleEventChains WRITE setVisibleEventChains NOTIFY optionsChanged)
        Q_PROPERTY(bool ShowMonteCarloEventChains READ showEventChains WRITE setShowEventChains NOTIFY optionsChanged)
        Q_PROPERTY(bool ShowReferenceData READ showReferenceData WRITE setShowReferenceData NOTIFY optionsChanged)
        Q_PROPERTY(bool LogScaleX READ isLogScaleX WRITE setIsLogScaleX NOTIFY optionsChanged)
        Q_PROPERTY(bool LogScaleYLeft READ isLogScaleYLeft WRITE setIsLogScaleYLeft NOTIFY optionsChanged)
        Q_PROPERTY(bool LogScaleYRight READ isLogScaleYRight WRITE setIsLogScaleYRight NOTIFY optionsChanged)
        Q_PROPERTY(int LineWidth READ lineWidth WRITE setLineWidth NOTIFY optionsChanged)
        Q_PROPERTY(int MarkerSize READ markerSize WRITE setMarkerSize NOTIFY optionsChanged)
        Q_PROPERTY(QColor ReferenceDataColor READ referenceDataColor WRITE setReferenceDataColor NOTIFY optionsChanged)
        
    public:
        StimulusClampProtocolPlot(QWidget *parent = 0);
        
        // Property getters.
        StimulusClampProtocol* protocol() const { return _protocol; }
        QString visibleSignalsYLeft() { return _visibleSignalsYLeft; }
        QString visibleSignalsYRight() { return _visibleSignalsYRight; }
        QString visibleVariableSetIndexes() { return _visibleVariableSetIndexes; }
        QString visibleRows() { return _visibleRows; }
        QString visibleColumns() { return _visibleColumns; }
        QString visibleEventChains() { return _visibleEventChains; }
        bool showEventChains() { return _showEventChains; }
        bool showReferenceData() { return _showReferenceData; }
        bool isLogScaleX() { return _isLogScaleX; }
        bool isLogScaleYLeft() { return _isLogScaleYLeft; }
        bool isLogScaleYRight() { return _isLogScaleYRight; }
        int lineWidth() { return _lineWidth; }
        int markerSize() { return _markerSize; }
        QColor referenceDataColor() { return _referenceDataColor; }
        bool drawingEnabled() const { return _drawingEnabled; }
        
        // Property setters.
        void setProtocol(StimulusClampProtocol *protocol) { _protocol = protocol; }
        void setVisibleSignalsYLeft(QString s) { _visibleSignalsYLeft = s; emit optionsChanged(); }
        void setVisibleSignalsYRight(QString s) { _visibleSignalsYRight = s; emit optionsChanged(); }
        void setVisibleVariableSetIndexes(QString s) { _visibleVariableSetIndexes = s; emit optionsChanged(); }
        void setVisibleRows(QString s) { _visibleRows = s; emit optionsChanged(); }
        void setVisibleColumns(QString s) { _visibleColumns = s; emit optionsChanged(); }
        void setVisibleEventChains(QString s) { _visibleEventChains = s; emit optionsChanged(); }
        void setShowEventChains(bool b) { _showEventChains = b; emit optionsChanged(); }
        void setShowReferenceData(bool b) { _showReferenceData = b; emit optionsChanged(); }
        void setIsLogScaleX(bool b) { _isLogScaleX = b; emit optionsChanged(); }
        void setIsLogScaleYLeft(bool b) { _isLogScaleYLeft = b; emit optionsChanged(); }
        void setIsLogScaleYRight(bool b) { _isLogScaleYRight = b; emit optionsChanged(); }
        void setLineWidth(int w) { _lineWidth = w; emit optionsChanged(); }
        void setMarkerSize(int w) { _markerSize = w; emit optionsChanged(); }
        void setReferenceDataColor(QColor color) { _referenceDataColor = color; emit optionsChanged(); }
        void setDrawingEnabled(bool b) { _drawingEnabled = b; }
        
        // Default minimum size hint is too big, so we'll redefine it here.
        QSize minimumSizeHint() const { return QSize(30, 30); }
        
        // Command menu.
        QMenu* getMenu();
        
    signals:
        void optionsChanged();
        
    public slots:
        void clearPlot();
        void plotProtocol();
        void autoscale();
        void editOptions();
        void exportVisibleToText();
        void exportVisibleToSvg();
        void exportMonteCarloEventChainsToDwt();
        
    protected slots:
        void _setVisibleSignalsYLeft(QString s) { setVisibleSignalsYLeft(s); }
        void _setVisibleSignalsYRight(QString s) { setVisibleSignalsYRight(s); }
        
    protected:
        QwtPlotCurve* addCurve(int yAxis, const QString &xTitle, const QString &yPostFix, const QString &yTitle, double *x, double *y, int npts, const QColor &color, QwtPlotCurve::CurveStyle style, bool isRawData = true);
        void mouseReleaseEvent(QMouseEvent *event);
        
    protected:
        // Properties.
        StimulusClampProtocol *_protocol;
        QString _visibleSignalsYLeft;
        QString _visibleSignalsYRight;
        QString _visibleVariableSetIndexes;
        QString _visibleRows;
        QString _visibleColumns;
        QString _visibleEventChains;
        bool _showEventChains;
        bool _showReferenceData;
        bool _isLogScaleX;
        bool _isLogScaleYLeft;
        bool _isLogScaleYRight;
        int _lineWidth;
        int _markerSize;
        QColor _referenceDataColor;
        bool _drawingEnabled;
        
        StimulusClampProtocolPlotPicker _picker;
        QwtPlotZoomer _zoomer;
        std::vector<QColor> _colorMap;
        std::vector<QwtPlotCurve*> _curves;
        std::vector<std::pair<QString, QString>> _curveTitlesXY;
    };
    
} // StimulusClampProtocol

#endif
