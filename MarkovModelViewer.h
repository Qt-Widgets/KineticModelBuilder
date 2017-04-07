/* --------------------------------------------------------------------------------
 * 3D viewer for a MarkovModel.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __MarkovModelViewer_H__
#define __MarkovModelViewer_H__

#include "QFont3D.h"
#include <QColor>
#include <QFont>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QMenu>
#include <QString>
#include <QVector3D>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace MarkovModel
{
    class MarkovModel;
    class State;
    class Transition;
    class BinaryElement;
    class Interaction;
    
    /* --------------------------------------------------------------------------------
     * 3D viewer for a MarkovModel.
     * -------------------------------------------------------------------------------- */
    class MarkovModelViewer : public QOpenGLWidget, protected QOpenGLFunctions
    {
        Q_OBJECT
        Q_PROPERTY(bool EvaluateExpressions READ isEvalExprs WRITE setIsEvalExprs NOTIFY optionsChanged)
        Q_PROPERTY(QString StimuliForExpressions READ exprStimuli WRITE setExprStimuli NOTIFY optionsChanged)
        Q_PROPERTY(int VariableSet READ variableSetIndex WRITE setVariableSetIndex NOTIFY optionsChanged)
        Q_PROPERTY(bool ShowBinaryElementTransitions READ showBinaryElementTransitions WRITE setShowBinaryElementTransitions NOTIFY optionsChanged)
        Q_PROPERTY(QSize GridSize READ gridSize WRITE setGridSize NOTIFY optionsChanged)
        Q_PROPERTY(QSizeF GridSpacing READ gridSpacing WRITE setGridSpacing NOTIFY optionsChanged)
        Q_PROPERTY(float GridLineWidth READ gridLineWidth WRITE setGridLineWidth NOTIFY optionsChanged)
        Q_PROPERTY(float NodeRadius READ nodeRadius WRITE setNodeRadius NOTIFY optionsChanged)
        Q_PROPERTY(float ConnectionLineWidth READ connectionLineWidth WRITE setConnectionLineWidth NOTIFY optionsChanged)
        Q_PROPERTY(float ConnectionTextOffset READ connectionTextOffset WRITE setConnectionTextOffset NOTIFY optionsChanged)
        Q_PROPERTY(float TransitionArrowSpacer READ transitionArrowSpacer WRITE setTransitionArrowSpacer NOTIFY optionsChanged)
        Q_PROPERTY(QSizeF TransitionArrowHeadSize READ transitionArrowHeadSize WRITE setTransitionArrowHeadSize NOTIFY optionsChanged)
        Q_PROPERTY(QFont HudFont READ hudFont WRITE setHudFont NOTIFY optionsChanged)
        Q_PROPERTY(QFont NodeFont READ nodeFont WRITE setNodeFont NOTIFY optionsChanged)
        Q_PROPERTY(QFont ConnectionFont READ connectionFont WRITE setConnectionFont NOTIFY optionsChanged)
        Q_PROPERTY(QColor BackgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor GridColor READ gridColor WRITE setGridColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor NodeColor READ nodeColor WRITE setNodeColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor NodeTextColor READ nodeTextColor WRITE setNodeTextColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor ConnectionColor READ connectionColor WRITE setConnectionColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor ConnectionTextColor READ connectionTextColor WRITE setConnectionTextColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor SelectedNodeColor READ selectedNodeColor WRITE setSelectedNodeColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor SelectedNodeTextColor READ selectedNodeTextColor WRITE setSelectedNodeTextColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor SelectedConnectionColor READ selectedConnectionColor WRITE setSelectedConnectionColor NOTIFY optionsChanged)
        Q_PROPERTY(QColor SelectedConnectionTextColor READ selectedConnectionTextColor WRITE setSelectedConnectionTextColor NOTIFY optionsChanged)
        
    public:
        MarkovModelViewer(QWidget *parent = 0);
        ~MarkovModelViewer();
        
        // Property getters.
        MarkovModel* model() const { return _model; }
        bool isEvalExprs() const { return _isEvalExprs; }
        QString exprStimuli() const { return _exprStimuli; }
        int variableSetIndex() const { return _variableSetIndex; }
        bool showBinaryElementTransitions() const { return _showBinaryElementTransitions; }
        QSize gridSize() const { return _gridSize; }
        QSizeF gridSpacing() const { return _gridSpacing; }
        float gridLineWidth() const { return _gridLineWidth; }
        float nodeRadius() const { return _nodeRadius; }
        float connectionLineWidth() const { return _connectionLineWidth; }
        float connectionTextOffset() const { return _connectionTextOffset; }
        float transitionArrowSpacer() const { return _transitionArrowSpacer; }
        QSizeF transitionArrowHeadSize() const { return _transitionArrowHeadSize; }
        QFont hudFont() const { return _hudFont; }
        QFont nodeFont() const { return _nodeTextRenderer.font(); }
        QFont connectionFont() const { return _connectionTextRenderer.font(); }
        QColor backgroundColor() const { return _backgroundColor; }
        QColor gridColor() const { return _gridColor; }
        QColor nodeColor() const { return _nodeColor; }
        QColor nodeTextColor() const { return _nodeTextColor; }
        QColor connectionColor() const { return _connectionColor; }
        QColor connectionTextColor() const { return _connectionTextColor; }
        QColor selectedNodeColor() const { return _selectedNodeColor; }
        QColor selectedNodeTextColor() const { return _selectedNodeTextColor; }
        QColor selectedConnectionColor() const { return _selectedConnectionColor; }
        QColor selectedConnectionTextColor() const { return _selectedConnectionTextColor; }
        
        // Property setters.
        void setModel(MarkovModel* model) { _model = model; repaint(); }
        void setIsEvalExprs(bool b) { _isEvalExprs = b; }
        void setExprStimuli(QString s) { _exprStimuli = s; }
        void setVariableSetIndex(int i) { _variableSetIndex = i; }
        void setShowBinaryElementTransitions(bool show) { _showBinaryElementTransitions = show; }
        void setGridSize(QSize size) { _gridSize = size; }
        void setGridSpacing(QSizeF spacing) { _gridSpacing = spacing; }
        void setGridLineWidth(float width) { _gridLineWidth = width; }
        void setNodeRadius(float radius) { _nodeRadius = radius; }
        void setConnectionLineWidth(float width) { _connectionLineWidth = width; }
        void setConnectionTextOffset(float offset) { _connectionTextOffset = offset; }
        void setTransitionArrowSpacer(float spacer) { _transitionArrowSpacer = spacer; }
        void setTransitionArrowHeadSize(QSizeF size) { _transitionArrowHeadSize = size; }
        void setHudFont(const QFont &font) { _hudFont = font; }
        void setNodeFont(const QFont &font) { _nodeTextRenderer.setFont(font, 1, 256); }
        void setConnectionFont(const QFont &font) { _connectionTextRenderer.setFont(font, 1, 256); }
        void setBackgroundColor(const QColor &color) { _backgroundColor = color; }
        void setGridColor(const QColor &color) { _gridColor = color; }
        void setNodeColor(const QColor &color) { _nodeColor = color; }
        void setNodeTextColor(const QColor &color) { _nodeTextColor = color; }
        void setConnectionColor(const QColor &color) { _connectionColor = color; }
        void setConnectionTextColor(const QColor &color) { _connectionTextColor = color; }
        void setSelectedNodeColor(const QColor &color) { _selectedNodeColor = color; }
        void setSelectedNodeTextColor(const QColor &color) { _selectedNodeTextColor = color; }
        void setSelectedConnectionColor(const QColor &color) { _selectedConnectionColor = color; }
        void setSelectedConnectionTextColor(const QColor &color) { _selectedConnectionTextColor = color; }
        
    public slots:
        void goToDefaultView();
        void editSelectedObject();
        void editModel();
        void editOptions();
        void clearModel();
        QMenu* editMenu(const QString &title = "Edit");
        void contextMenu(const QPoint &pos) { editMenu()->popup(mapToGlobal(pos)); }
        void addState();
        void addTransition() { if(_model) _flags = ADDING_NEW_CONNECTION; }
        void addBinaryElement();
        void addInteraction() { if(_model) _flags = ADDING_NEW_CONNECTION; }
        void removeSelectedObject();
        
    signals:
        void optionsChanged();
        void selectedObjectChanged(QObject*);
        
    protected:
        void initializeGL() Q_DECL_OVERRIDE;
        void resizeGL(int w, int h) Q_DECL_OVERRIDE;
        void paintGL() Q_DECL_OVERRIDE;
        
        void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
        void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
        void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        
        void drawGrid();
        void drawState(State *state);
        void drawTransition(Transition *transition);
        void drawBinaryElement(BinaryElement *binaryElement);
        void drawInteraction(Interaction *interaction);
        void drawNode(const QVector3D &center, float radius, const QColor &color, const QString &text, const QColor &textColor);
        void goToAB(const QVector3D &a, const QVector3D &b);
        
        void selectObject(int x, int y);
        void pickRay(int x, int y, QVector3D &rayOrigin, QVector3D &rayDirection);
        float intersectRayAndSphere(const QVector3D &rayOrigin, const QVector3D &rayDirection, const QVector3D &sphereCenter, float sphereRadius);
        float intersectRayAndPlane(const QVector3D &rayOrigin, const QVector3D &rayDirection, const QVector3D &pointOnPlane, const QVector3D &planeNormal);
        QVector3D pickPointInSelectionPlane(int x, int y, bool snapToGrid = true);
        
        void perspective(double fovy, double aspect, double zNear, double zFar);
        void lookAt(const QVector3D &eye, const QVector3D &center, const QVector3D &up);
        QVector3D pixels2World(QVector3D pixel, int *viewport, double *projection, double *modelview);
        QVector3D world2Pixels(QVector3D world, int *viewport, double *projection, double *modelview);
        
    protected:
        // Properties.
        MarkovModel *_model;
        bool _isEvalExprs;
        QString _exprStimuli;
        int _variableSetIndex;
        bool _showBinaryElementTransitions;
        QSize _gridSize;
        QSizeF _gridSpacing;
        float _gridLineWidth;
        float _nodeRadius;
        float _connectionLineWidth;
        float _connectionTextOffset;
        float _transitionArrowSpacer;
        QSizeF _transitionArrowHeadSize;
        QFont _hudFont;
        QFont3D _nodeTextRenderer;
        QFont3D _connectionTextRenderer;
        QColor _backgroundColor;
        QColor _gridColor;
        QColor _nodeColor;
        QColor _nodeTextColor;
        QColor _connectionColor;
        QColor _connectionTextColor;
        QColor _selectedNodeColor;
        QColor _selectedNodeTextColor;
        QColor _selectedConnectionColor;
        QColor _selectedConnectionTextColor;
        
        // Internal state.
        QObject *_selectedObject;
        QVector3D _eye, _center, _up;
        QPointF _prevMousePos;
        bool _hasPrevMousePos;
        int _flags;
        enum Flags { ADDING_NEW_NODE = 1, ADDING_NEW_CONNECTION };
    };
    
} // MarkovModel

#endif
