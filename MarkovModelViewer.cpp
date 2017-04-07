/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "MarkovModelViewer.h"
#include "MarkovModel.h"
#include "MarkovModelPropertyEditor.h"
#include "QObjectPropertyEditor.h"
#include <cmath>
#ifdef MACX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <QByteArray>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHash>
#include <QKeyEvent>
#include <QList>
#include <QMessageBox>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <Eigen/Dense>

namespace MarkovModel
{
    MarkovModelViewer::MarkovModelViewer(QWidget *parent) :
    QOpenGLWidget(parent),
    _model(0),
    _isEvalExprs(false),
    _exprStimuli("T: 298.15, V: 0"),
    _showBinaryElementTransitions(true),
    _gridSize(30, 30),
    _gridSpacing(1, 1),
    _gridLineWidth(1),
    _nodeRadius(0.4),
    _connectionLineWidth(3),
    _connectionTextOffset(0.08),
    _transitionArrowSpacer(0.16),
    _transitionArrowHeadSize(0.2, 0.2),
    _hudFont("Sans", 10, QFont::Normal),
    _backgroundColor(200, 200, 200),
    _gridColor(220, 220, 220),
    _nodeColor(25, 150, 200),
    _nodeTextColor(255, 255, 255),
    _connectionColor(128, 0, 128),
    _connectionTextColor(0, 0, 0),
    _selectedNodeColor(255, 64, 64),//(128, 128, 200),
    _selectedNodeTextColor(255, 255, 255),
    _selectedConnectionColor(255, 64, 64),
    _selectedConnectionTextColor(0, 0, 0),
    _selectedObject(0),
    _hasPrevMousePos(false),
    _flags(0)
    {
        connect(this, SIGNAL(optionsChanged()), this, SLOT(repaint()));
        
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
    }
    
    MarkovModelViewer::~MarkovModelViewer()
    {
        makeCurrent();
    }
    
    void MarkovModelViewer::goToDefaultView()
    {
        _eye = QVector3D(0, 0, 50);
        _center = QVector3D(0, 0, 0);
        _up = QVector3D(0, 1, 0);
        repaint();
    }
    
    void MarkovModelViewer::editSelectedObject()
    {
        if(!_selectedObject)
            return;
        
        QObjectPropertyEditor::QObjectPropertyModel propertyModel;
        propertyModel.setObject(_selectedObject);
        
        QList<QByteArray> propertyNames = QObjectPropertyEditor::getObjectPropertyNames(_selectedObject);
        propertyNames.removeOne("objectName");
        propertyModel.setPropertyNames(propertyNames);
        
        QObjectPropertyEditor::QObjectPropertyEditor *editor = new QObjectPropertyEditor::QObjectPropertyEditor();
        editor->setModel(&propertyModel);
        
        QDialog dialog(this);
        dialog.setWindowTitle(_selectedObject->metaObject()->className());
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(editor);
        dialog.show();
        editor->resizeColumnToContents(0);
        dialog.exec();
    }
    
    void MarkovModelViewer::editModel()
    {
        if(!_model)
            return;
        QDialog dialog(this);
        dialog.setWindowTitle("Model Parameters");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(new MarkovModelPropertyEditor(_model));
        dialog.show();
        QPoint topLeft = mapToGlobal(QPoint(0, 0));
        dialog.setGeometry(topLeft.x(), topLeft.y(), width(), height());
        dialog.exec();
    }
    
    void MarkovModelViewer::editOptions()
    {
        QObjectPropertyEditor::QObjectPropertyModel propertyModel;
        propertyModel.setObject(this);
        
        QList<QByteArray> propertyNames = QObjectPropertyEditor::getObjectPropertyNames(this);
        QWidget widget;
        QList<QByteArray> widgetPropertyNames = QObjectPropertyEditor::getObjectPropertyNames(&widget);
        foreach(const QByteArray &propertyName, widgetPropertyNames)
            propertyNames.removeOne(propertyName);
        propertyModel.setPropertyNames(propertyNames);
        
        QObjectPropertyEditor::QObjectPropertyEditor *editor = new QObjectPropertyEditor::QObjectPropertyEditor();
        editor->setModel(&propertyModel);
        
        QDialog dialog(this);
        dialog.setWindowTitle("Model Viewer Options");
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        layout->addWidget(editor);
        dialog.show();
        editor->resizeColumnToContents(0);
        QPoint topLeft = mapToGlobal(QPoint(0, 0));
        dialog.setGeometry(topLeft.x(), topLeft.y(), width(), height());
        dialog.exec();
    }
    
    void MarkovModelViewer::clearModel()
    {
        if(!_model)
            return;
        if(QMessageBox::question(this, "Clear Model", "Clear model " + _model->objectName() + "?",
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            return;
        _model->clear();
        repaint();
    }
    
    QMenu* MarkovModelViewer::editMenu(const QString &title)
    {
        QMenu *menu = new QMenu(title);
        menu->addAction("Reset View", this, SLOT(goToDefaultView()));
        if(_model) {
            menu->addSeparator();
            menu->addAction("Model Parameters", this, SLOT(editModel()));
            menu->addSeparator();
            menu->addAction("Add State", this, SLOT(addState()));
            menu->addAction("Add Transition", this, SLOT(addTransition()));
            menu->addAction("Add Binary Element", this, SLOT(addBinaryElement()));
            menu->addAction("Add Interaction", this, SLOT(addInteraction()));
            menu->addSeparator();
            menu->addAction("Remove Selected Object", this, SLOT(removeSelectedObject()));
            menu->addSeparator();
            menu->addAction("Clear Model", this, SLOT(clearModel()));
        }
        menu->addSeparator();
        menu->addAction("Options", this, SLOT(editOptions()));
        return menu;
    }
    
    void MarkovModelViewer::addState()
    {
        if(!_model)
            return;
        QPoint mousePos = mapFromGlobal(QCursor::pos());
        State *state = new State(_model);
        state->position = QVector3D(pickPointInSelectionPlane(mousePos.x(), mousePos.y()));
        _selectedObject = state;
        emit selectedObjectChanged(_selectedObject);
        _flags = ADDING_NEW_NODE;
        repaint();
        setMouseTracking(true);
    }
    
    void MarkovModelViewer::addBinaryElement()
    {
        if(!_model)
            return;
        QPoint mousePos = mapFromGlobal(QCursor::pos());
        BinaryElement *binaryElement = new BinaryElement(_model);
        binaryElement->position = QVector3D(pickPointInSelectionPlane(mousePos.x(), mousePos.y()));
        _selectedObject = binaryElement;
        emit selectedObjectChanged(_selectedObject);
        _flags = ADDING_NEW_NODE;
        repaint();
        setMouseTracking(true);
    }
    
    void MarkovModelViewer::removeSelectedObject()
    {
        if(!_selectedObject)
            return;
        QString title("Delete selected object?");
        QString text("Delete " + _selectedObject->objectName() + "?");
        if(QMessageBox::question(this, title, text, QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
            return;
        delete _selectedObject;
        _selectedObject = 0;
        emit selectedObjectChanged(_selectedObject);
        repaint();
    }

    void MarkovModelViewer::initializeGL()
    {
        initializeOpenGLFunctions();
        glClearColor(_backgroundColor.redF(), _backgroundColor.greenF(), _backgroundColor.blueF(), _backgroundColor.alphaF());
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
        goToDefaultView();
        // We need a valid OpenGL context before building the 3D font display lists.
        _nodeTextRenderer.setFont(QFont("Sans", 20, QFont::Bold), 1, 256);
        _connectionTextRenderer.setFont(QFont("Sans", 16, QFont::Bold), 1, 256);
    }
    
    void MarkovModelViewer::resizeGL(int w, int h)
    {
        glViewport(0, 0, width(), height());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        perspective(10, float(w) / h, 2, 200);
        glMatrixMode(GL_MODELVIEW);
    }

    void MarkovModelViewer::paintGL()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        lookAt(_eye, _center, _up);
        drawGrid();
        if(!_model)
            return;
//        if(parseExpressions())
//            model()->constrainVariables(MarkovModel::parseAttributes(parserStimuli()));
        foreach(State *state, _model->findChildren<State*>(QString(), Qt::FindDirectChildrenOnly))
            drawState(state);
        foreach(Transition *transition, _model->findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly))
            drawTransition(transition);
        foreach(BinaryElement *binaryElement, _model->findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly))
            drawBinaryElement(binaryElement);
        foreach(Interaction *interaction, _model->findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly))
            drawInteraction(interaction);
//        hud
//        QFontMetricsF fm(m_hudFont);
//        QRectF bbox = fm.tightBoundingRect(m_model->objectName());
//        float x = bbox.bottomLeft().x();
//        float y = bbox.bottomLeft().y();
//        float h = bbox.height();
//        glPushAttrib(GL_COLOR_BUFFER_BIT);
//        glColor3f(0, 0, 0);
//        renderText(2 - x, 2 - y + h, m_model->objectName(), m_hudFont);
//        glPopAttrib();
    }

    void MarkovModelViewer::keyPressEvent(QKeyEvent *event)
    {
        if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
            if(_selectedObject) {
                removeSelectedObject();
                return;
            }
        }
        QOpenGLWidget::keyPressEvent(event);
    }
    
    void MarkovModelViewer::mousePressEvent(QMouseEvent *event)
    {
        if(event->button() == Qt::LeftButton) {
            if(_selectedObject && (_flags & ADDING_NEW_NODE)) {
                if(State *state = qobject_cast<State*>(_selectedObject)) {
                    state->position = pickPointInSelectionPlane(event->x(), event->y());
                    _flags = 0;
                    repaint();
                    return;
                } else if(BinaryElement *binaryElement = qobject_cast<BinaryElement*>(_selectedObject)) {
                    binaryElement->position = pickPointInSelectionPlane(event->x(), event->y());
                    _flags = 0;
                    repaint();
                    return;
                }
            } else if(_flags & ADDING_NEW_CONNECTION) {
                if(State *from = qobject_cast<State*>(_selectedObject)) {
                    selectObject(event->x(), event->y());
                    if(State *to = qobject_cast<State*>(_selectedObject)) {
                        if(_model && from && to && (from != to) && !_model->findTransition(from, to)) {
                            _selectedObject = new Transition(_model, from, to);
                            emit selectedObjectChanged(_selectedObject);
                            _flags = 0;
                            repaint();
                            return;
                        }
                    }
                }
                if(BinaryElement *A = qobject_cast<BinaryElement*>(_selectedObject)) {
                    selectObject(event->x(), event->y());
                    if(BinaryElement *B = qobject_cast<BinaryElement*>(_selectedObject)) {
                        if(_model && A && B && (A != B) && !_model->findInteraction(A, B)) {
                            _selectedObject = new Interaction(_model, A, B);
                            emit selectedObjectChanged(_selectedObject);
                            _flags = 0;
                            repaint();
                            return;
                        }
                    }
                }
            }
            selectObject(event->x(), event->y());
            setMouseTracking(true);
            repaint();
            return;
        }
        QOpenGLWidget::mousePressEvent(event);
    }
    
    void MarkovModelViewer::mouseReleaseEvent(QMouseEvent *event)
    {
        if(event->button() == Qt::LeftButton) {
            setMouseTracking(false);
            _hasPrevMousePos = false;
            return;
        }
        QOpenGLWidget::mousePressEvent(event);
    }
    
    void MarkovModelViewer::mouseMoveEvent(QMouseEvent *event)
    {
        if(_selectedObject && ((event->buttons() & Qt::LeftButton) || (_flags & ADDING_NEW_NODE))) {
            if(State *state = qobject_cast<State*>(_selectedObject)) {
                state->position = pickPointInSelectionPlane(event->x(), event->y());
                repaint();
                return;
            } else if(BinaryElement *binaryElement = qobject_cast<BinaryElement*>(_selectedObject)) {
                binaryElement->position = pickPointInSelectionPlane(event->x(), event->y());
                repaint();
                return;
            }
        } else {
            float x = event->x();
            float y = event->y();
            float dx = (_hasPrevMousePos ? x - _prevMousePos.x() : 0);
            float dy = (_hasPrevMousePos ? y - _prevMousePos.y() : 0);
            _prevMousePos = QPointF(x, y);
            _hasPrevMousePos = true;
            if(event->buttons() & Qt::LeftButton) {
                // Rotate scene.
                QVector3D zhat = _eye - _center;
                zhat.normalize();
                QVector3D yhat = _up;
                QVector3D xhat = QVector3D::crossProduct(yhat, zhat);
                yhat = QVector3D::crossProduct(zhat, xhat);
                QVector3D rotationAxis = yhat * dx + xhat * dy;
                float n = rotationAxis.length();
                if(n > 1e-5) {
                    rotationAxis.normalize();
                    float radians = n / width() * M_PI;
                    // Rotate eye about center around rotation axis.
                    float a = cos(radians / 2);
                    float s = -sin(radians / 2);
                    float b = rotationAxis.x() * s;
                    float c = rotationAxis.y() * s;
                    float d = rotationAxis.z() * s;
                    float rotationMatrix[9] = {
                        a*a+b*b-c*c-d*d, 2*(b*c-a*d), 2*(b*d+a*c),
                        2*(b*c+a*d), a*a+c*c-b*b-d*d, 2*(c*d-a*b),
                        2*(b*d-a*c), 2*(c*d+a*b), a*a+d*d-b*b-c*c};
                    _eye -= _center; // Shift center to origin so can rotate eye about axis through the origin.
                    // Rotate eye around rotation axis.
                    float ex = _eye.x() * rotationMatrix[0] + _eye.y() * rotationMatrix[1] + _eye.z() * rotationMatrix[2];
                    float ey = _eye.x() * rotationMatrix[3] + _eye.y() * rotationMatrix[4] + _eye.z() * rotationMatrix[5];
                    float ez = _eye.x() * rotationMatrix[6] + _eye.y() * rotationMatrix[7] + _eye.z() * rotationMatrix[8];
                    _eye = QVector3D(ex, ey, ez);
                    _eye += _center; // shift back to center.
                    repaint();
                }
                return;
            } else if (event->buttons() & Qt::MiddleButton) {
                // Translate the scene.
                QVector3D pos = pickPointInSelectionPlane(x, y, false);
                QVector3D prevPos = pickPointInSelectionPlane(x - dx, y - dy, false);
                QVector3D translation = pos - prevPos;
                _center -= translation;
                _eye -= translation;
                repaint();
                return;
            }
        }
        QOpenGLWidget::mouseMoveEvent(event);
    }
    
    void MarkovModelViewer::wheelEvent(QWheelEvent *event)
    {
#if QT_VERSION >= 0x050000
        float degrees = event->angleDelta().y() / 8;
#else
        float degrees = event->delta() / 8;
#endif
        float steps = -degrees / 15;  // Most mouse types work in steps of 15 degrees.
        QVector3D view = _center - _eye;
        float n = view.length();
        view.normalize();
        if(steps > 0)
            n *= (steps * 0.75);
        else if(steps < 0)
            n /= (-steps * 0.75);
        if(n < 1)
            n = 1;
        else if(n > 120)
            n = 120;
        view *= n;
        _eye = _center - view;
        repaint();
    }
    
    void MarkovModelViewer::mouseDoubleClickEvent(QMouseEvent *event)
    {
        if(event->button() == Qt::LeftButton) {
            selectObject(event->x(), event->y());
            if(_selectedObject) {
                editSelectedObject();
                return;
            }
        }
        QOpenGLWidget::mouseDoubleClickEvent(event);
    }
    
    void MarkovModelViewer::drawGrid()
    {
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glPushAttrib(GL_LINE_BIT);
        glColor4f(_gridColor.redF(), _gridColor.greenF(), _gridColor.blueF(), _gridColor.alphaF());
        glLineWidth(_gridLineWidth);
        int nx = _gridSize.width();
        int ny = _gridSize.height();
        float dx = _gridSpacing.width();
        float dy = _gridSpacing.height();
        float x0 = -nx * dx / 2;
        float y0 = -ny * dy / 2;
        float x1 = x0 + nx * dx;
        float y1 = y0 + ny * dy;
        glBegin(GL_LINES);
        for(int i = 0; i <= nx; ++i) {
            glVertex3f(x0 + i * dx, y0, 0);
            glVertex3f(x0 + i * dx, y1, 0);
        }
        for(int i = 0; i <= ny; ++i) {
            glVertex3f(x0, y0 + i * dy, 0);
            glVertex3f(x1, y0 + i * dy, 0);
        }
        glEnd();
        glPopAttrib();
        glPopAttrib();
    }
    
    void MarkovModelViewer::drawState(State *state)
    {
        if(!state)
            return;
        bool isSelected = (state == _selectedObject);
        QColor &nodeColor = (isSelected ? _selectedNodeColor : _nodeColor);
        QColor &nodeTextColor = (isSelected ? _selectedNodeTextColor : _nodeTextColor);
        drawNode(state->position, _nodeRadius, nodeColor, state->name(), nodeTextColor);
    }
    
    void MarkovModelViewer::drawTransition(Transition *transition)
    {
        if(!transition || !transition->from() || !transition->to())
            return;
        QVector3D a = transition->from()->position;
        QVector3D b = transition->to()->position;
        float length = (b - a).length();
        if(length < 2.5 * _nodeRadius)
            return;
        bool isSelected = (transition == _selectedObject);
        QColor &connectionColor = (isSelected ? _selectedConnectionColor : _connectionColor);
        QColor &connectionTextColor = (isSelected ? _selectedConnectionTextColor : _connectionTextColor);
        glPushMatrix();
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glPushAttrib(GL_LINE_BIT);
        goToAB(a, b);
        // Draw arrow.
        glColor4f(connectionColor.redF(), connectionColor.greenF(), connectionColor.blueF(), connectionColor.alphaF());
        glLineWidth(_connectionLineWidth + (isSelected ? 1 : 0));
        float nodeOffset = 1.25 * _nodeRadius;
        float x0 = nodeOffset;
        float x1 = length - nodeOffset;
        float y0 = _transitionArrowSpacer / 2;
        float z0 = 0.05; // Bring in front of grid lines if on grid plane.
        float w = _transitionArrowHeadSize.width();
        float h = _transitionArrowHeadSize.height();
        glBegin(GL_LINE_STRIP);
        glVertex3f(x0, y0, z0);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1 - w, y0 + h, z0);
        glEnd();
        // Draw text above or below arrow.
        QVector3D view = (_center - _eye).normalized();
        QVector3D up = _up.normalized();
        bool isLeftToRight = (QVector3D::dotProduct(QVector3D::crossProduct(view, up), b - a) >= 0 ? true : false);
        if(QVector3D::dotProduct(up, b - a) <= -length)
            isLeftToRight = false;
        if(!isLeftToRight) {
            glPopMatrix();
            glPushMatrix();
            goToAB(b, a);
        }
        // Text above arrow.
        QString text = transition->rate();
//        if(parseExpressions()) {
//            try {
//                text = QString::number(transition->rate(model()->exprParser()));
//            } catch(...) {
//            }
//        }
        QVector3D position(length / 2, y0 + _connectionTextOffset, 0);
        int align = Qt::AlignHCenter | Qt::AlignBottom;
        float scale = float(connectionFont().pointSize()) / 1200;
        if(!isLeftToRight) {
            // Text below arrow.
            position.setY(-y0 - _connectionTextOffset);
            align = Qt::AlignHCenter | Qt::AlignTop;
        }
        glColor4f(connectionTextColor.redF(), connectionTextColor.greenF(), connectionTextColor.blueF(), connectionTextColor.alphaF());
        _connectionTextRenderer.print(text, position, align, scale);
        glPopAttrib(); // GL_LINE_BIT
        glPopAttrib(); // GL_COLOR_BUFFER_BIT
        glPopMatrix();
    }
    
    void MarkovModelViewer::drawBinaryElement(BinaryElement *binaryElement)
    {
        if(!binaryElement)
            return;
        bool isSelected = (binaryElement == _selectedObject);
        QColor &nodeColor = (isSelected ? _selectedNodeColor : _nodeColor);
        QColor &nodeTextColor = (isSelected ? _selectedNodeTextColor : _nodeTextColor);
        drawNode(binaryElement->position, _nodeRadius, nodeColor, binaryElement->name(), nodeTextColor);
        if(_showBinaryElementTransitions) {
            QVector3D view = (_center - _eye);
            QVector3D up = _up.normalized() * (1 + 1.25 * _nodeRadius);
            QVector3D right = QVector3D::crossProduct(view, up).normalized() * _nodeRadius;
            State s01_0, s01_1, s10_1, s10_0;
            Transition k01(0, &s01_0, &s01_1), k10(0, &s10_1, &s10_0);
            s01_0.position = binaryElement->position - right - up;
            s01_1.position = binaryElement->position - right + up;
            s10_1.position = binaryElement->position + right + up;
            s10_0.position = binaryElement->position + right - up;
            k01.setRate(binaryElement->rate01());
            k10.setRate(binaryElement->rate10());
            drawTransition(&k01);
            drawTransition(&k10);
        }
    }
    
    void MarkovModelViewer::drawInteraction(Interaction *interaction)
    {
        if(!interaction || !interaction->A() || !interaction->B())
            return;
        QVector3D a = interaction->A()->position;
        QVector3D b = interaction->B()->position;
        float length = (b - a).length();
        if(length < 2.5 * _nodeRadius)
            return;
        bool isSelected = (interaction == _selectedObject);
        QColor &connectionColor = (isSelected ? _selectedConnectionColor : _connectionColor);
        QColor &connectionTextColor = (isSelected ? _selectedConnectionTextColor : _connectionTextColor);
        glPushMatrix();
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glPushAttrib(GL_LINE_BIT);
        goToAB(a, b);
        // Draw line.
        glColor4f(connectionColor.redF(), connectionColor.greenF(), connectionColor.blueF(), connectionColor.alphaF());
        glLineWidth(_connectionLineWidth + (isSelected ? 1 : 0));
        float nodeOffset = 1.25 * _nodeRadius;
        if(_showBinaryElementTransitions)
            nodeOffset = 2.5 * _nodeRadius;
        float x0 = nodeOffset;
        float x1 = length - nodeOffset;
        float y0 = 0;
        float z0 = 0.05; // Bring in front of grid lines if on grid plane.
        glBegin(GL_LINES);
        glVertex3f(x0, y0, z0);
        glVertex3f(x1, y0, z0);
        glEnd();
        // Draw text above line.
        QVector3D view = (_center - _eye).normalized();
        QVector3D up = _up.normalized();
        bool isLeftToRight = (QVector3D::dotProduct(QVector3D::crossProduct(view, up), b - a) >= 0 ? true : false);
        if(QVector3D::dotProduct(up, b - a) <= -length)
            isLeftToRight = false;
        if(!isLeftToRight) {
            glPopMatrix();
            glPushMatrix();
            goToAB(b, a);
        }
        // Text above line.
        QString text11 = interaction->factor11();
        QString textA1 = interaction->factorA1();
        QString text1B = interaction->factor1B();
//        if(parseExpressions()) {
//            try {
//                text11 = QString::number(interaction->factor11(model()->exprParser()));
//            } catch(...) {
//            }
//            try {
//                textA1 = QString::number(interaction->factorA1(model()->exprParser()));
//            } catch(...) {
//            }
//            try {
//                text1B = QString::number(interaction->factor1B(model()->exprParser()));
//            } catch(...) {
//            }
//        }
        QString text = isLeftToRight ? (textA1 + ", " + text11 + ", " + text1B) : (text1B + ", " + text11 + ", " + textA1);
        QVector3D position(length / 2, _connectionTextOffset, 0);
        int align = Qt::AlignHCenter | Qt::AlignBottom;
        float scale = float(connectionFont().pointSize()) / 1200;
        glColor4f(connectionTextColor.redF(), connectionTextColor.greenF(), connectionTextColor.blueF(), connectionTextColor.alphaF());
        _connectionTextRenderer.print(text, position, align, scale);
        glPopAttrib(); // GL_LINE_BIT
        glPopAttrib(); // GL_COLOR_BUFFER_BIT
        glPopMatrix();
    }
    
    void MarkovModelViewer::drawNode(const QVector3D &center, float radius, const QColor &color, const QString &text, const QColor &textColor)
    {
        glPushMatrix();
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        glTranslatef(center.x(), center.y(), center.z());
        // Draw node as a shaded sphere.
        GLUquadric *quadric = gluNewQuadric();
        if(quadric) {
            glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
            glEnable(GL_LIGHTING);
            gluSphere(quadric, radius, 32, 32);
            glDisable(GL_LIGHTING);
            gluDeleteQuadric(quadric);
        }
        // Draw billboard text in front of node.
        if(!text.isEmpty()) {
            // Move to just in front of node.
            QVector3D view = (_center - _eye).normalized() * radius * 1.1;
            QVector3D up = _up.normalized();
            QVector3D right = QVector3D::crossProduct(view, up).normalized();
            glTranslatef(-view.x(), -view.y(), -view.z());
            goToAB(QVector3D(0, 0, 0), right);
            // Draw 3D text.
            glColor4f(textColor.redF(), textColor.greenF(), textColor.blueF(), textColor.alphaF());
            float scale = float(nodeFont().pointSize()) / 1200;
            _nodeTextRenderer.print(text, QVector3D(0, 0, 0), Qt::AlignCenter, scale);
        }
        glPopAttrib(); // GL_COLOR_BUFFER_BIT
        glPopMatrix();
    }
    
    void MarkovModelViewer::goToAB(const QVector3D &a, const QVector3D &b)
    {
        // x: Along ab.
        // y: Perpendicular to ab and camera view.
        // z: Along the camera view.
        QVector3D x, y, z;
        QVector3D view = (_center - _eye).normalized();
        x = (b - a).normalized();
        y = QVector3D::crossProduct(x, view).normalized();
        z = QVector3D::crossProduct(x, y).normalized();
        // Rotation to (x, y, z) and translation to a.
        float transformationMatrix[16] = {
            x.x(), x.y(), x.z(), 0,
            y.x(), y.y(), y.z(), 0,
            z.x(), z.y(), z.z(), 0,
            a.x(), a.y(), a.z(), 1
        };
        glMultMatrixf(transformationMatrix);
    }
    
    void MarkovModelViewer::selectObject(int x, int y)
    {
        QVector3D rayOrigin, rayDirection;
        pickRay(x, y, rayOrigin, rayDirection);
        rayDirection.normalize();
        QVector3D view = (_center - _eye).normalized();
        float alpha = 0, alphaMin = 0;
        QObject *prevSelectedObject = _selectedObject;
        _selectedObject = 0;
        if(_model) {
            foreach(State *state, _model->findChildren<State*>(QString(), Qt::FindDirectChildrenOnly)) {
                alpha = intersectRayAndSphere(rayOrigin, rayDirection, state->position, _nodeRadius);
                if(alpha >= 0 && (!_selectedObject || alpha < alphaMin)) {
                    _selectedObject = state;
                    alphaMin = alpha;
                }
            }
            foreach(Transition *transition, _model->findChildren<Transition*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(transition->from() && transition->to()) {
                    QVector3D a = transition->from()->position;
                    QVector3D b = transition->to()->position;
                    QVector3D ab = b - a;
                    float length = ab.length();
                    ab.normalize();
                    QVector3D offset = QVector3D::crossProduct(ab, view);
                    offset *= _transitionArrowSpacer;
                    a += offset;
                    b += offset;
                    float radius = _transitionArrowSpacer;
                    float dl = 1.0 / (4 * ceil(length / radius));
                    for(float j = 0; j <= 1; j += dl) {
                        QVector3D sphereCenter = a + (ab * (j * length));
                        alpha = intersectRayAndSphere(rayOrigin, rayDirection, sphereCenter, radius);
                        if(alpha >= 0 && (!_selectedObject || alpha < alphaMin)) {
                            _selectedObject = transition;
                            alphaMin = alpha;
                        }
                    }
                }
            }
            foreach(BinaryElement *binaryElement, _model->findChildren<BinaryElement*>(QString(), Qt::FindDirectChildrenOnly)) {
                alpha = intersectRayAndSphere(rayOrigin, rayDirection, binaryElement->position, _nodeRadius);
                if(alpha >= 0 && (!_selectedObject || alpha < alphaMin)) {
                    _selectedObject = binaryElement;
                    alphaMin = alpha;
                }
            }
            foreach(Interaction *interaction, _model->findChildren<Interaction*>(QString(), Qt::FindDirectChildrenOnly)) {
                if(interaction->A() && interaction->B()) {
                    QVector3D a = interaction->A()->position;
                    QVector3D b = interaction->B()->position;
                    QVector3D ab = b - a;
                    float length = ab.length();
                    ab.normalize();
                    float radius = _transitionArrowSpacer;
                    float dl = 1.0 / (4 * ceil(length / radius));
                    for(float j = 0; j <= 1; j += dl) {
                        QVector3D sphereCenter = a + (ab * (j * length));
                        alpha = intersectRayAndSphere(rayOrigin, rayDirection, sphereCenter, radius);
                        if(alpha >= 0 && (!_selectedObject || alpha < alphaMin)) {
                            _selectedObject = interaction;
                            alphaMin = alpha;
                        }
                    }
                }
            }
        }
        if(_selectedObject != prevSelectedObject)
            emit selectedObjectChanged(_selectedObject);
    }
    
    void MarkovModelViewer::pickRay(int x, int y, QVector3D &rayOrigin, QVector3D &rayDirection)
    {
        makeCurrent();
        int viewport[4] = {0, 0, width(), height()};
        double projection[16];
        double modelview[16];
        //glGetIntegerv(GL_VIEWPORT, viewport.data());
        glGetDoublev(GL_PROJECTION_MATRIX, projection);
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
        rayOrigin = pixels2World(QVector3D(x, y, 0), viewport, projection, modelview);
        rayDirection = pixels2World(QVector3D(x, y, 1), viewport, projection, modelview) - rayOrigin;
    }
    
    float MarkovModelViewer::intersectRayAndSphere(const QVector3D &rayOrigin, const QVector3D &rayDirection,
                                                   const QVector3D &sphereCenter, float sphereRadius)
    {
        QVector3D L = sphereCenter - rayOrigin;
        float tca = QVector3D::dotProduct(L, rayDirection);
        if(tca < 0) return -1;
        float d2 = QVector3D::dotProduct(L, L) - (tca * tca);
        float r2 = sphereRadius * sphereRadius;
        if(d2 > r2) return -1;
        float thc = sqrt(r2 - d2);
        return tca - thc;
    }
    
    float MarkovModelViewer::intersectRayAndPlane(const QVector3D &rayOrigin, const QVector3D &rayDirection,
                                                  const QVector3D &pointOnPlane, const QVector3D &planeNormal)
    {
        float numerator = QVector3D::dotProduct(planeNormal, pointOnPlane - rayOrigin);
        float denominator = QVector3D::dotProduct(planeNormal, rayDirection);
        return (fabs(denominator) > 1e-5 ? numerator / denominator : -1);
    }
    
    QVector3D MarkovModelViewer::pickPointInSelectionPlane(int x, int y, bool snapToGrid)
    {
        QVector3D rayOrigin, rayDirection;
        pickRay(x, y, rayOrigin, rayDirection);
        rayDirection.normalize();
        QVector3D view = (_center - _eye).normalized();
        QVector3D pointOnPlane = _center;
        if(_selectedObject) {
            if(State *state = qobject_cast<State*>(_selectedObject))
                pointOnPlane = state->position;
            else if(BinaryElement *binaryElement = qobject_cast<BinaryElement*>(_selectedObject))
                pointOnPlane = binaryElement->position;
        }
        float t = intersectRayAndPlane(rayOrigin, rayDirection, pointOnPlane, view);
        if(t >= 0) {
            QVector3D pt = rayOrigin + (rayDirection * t);
            if(snapToGrid) {
                pt.setX(round(pt.x()));
                pt.setY(round(pt.y()));
                pt.setZ(round(pt.z()));
            }
            return pt;
        }
        return _center;
    }
    
    void MarkovModelViewer::perspective(double fovy, double aspect, double zNear, double zFar)
    {
        double ymax = zNear * tan(fovy * M_PI / 360.0);
        double ymin = -ymax;
        double xmin = ymin * aspect;
        double xmax = ymax * aspect;
        glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
    }
    
    void MarkovModelViewer::lookAt(const QVector3D &eye, const QVector3D &center, const QVector3D &up)
    {
        QVector3D forward = (center - eye).normalized();
        QVector3D side = QVector3D::crossProduct(forward, up).normalized();
        QVector3D nup = QVector3D::crossProduct(side, forward).normalized();
        
        float m[4][4];
        m[0][0] = side.x(); m[0][1] = nup.x(); m[0][2] = -forward.x(); m[0][3] = 0;
        m[1][0] = side.y(); m[1][1] = nup.y(); m[1][2] = -forward.y(); m[1][3] = 0;
        m[2][0] = side.z(); m[2][1] = nup.z(); m[2][2] = -forward.z(); m[2][3] = 0;
        m[3][0] = 0;        m[3][1] = 0;       m[3][2] = 0;            m[3][3] = 1;
        
        glMultMatrixf(&m[0][0]);
        glTranslated(-eye.x(), -eye.y(), -eye.z());
    }
    
    QVector3D MarkovModelViewer::pixels2World(QVector3D pixel, int *viewport, double *projection, double *modelview)
    {
        Eigen::Map<Eigen::Matrix4d> P(projection);
        Eigen::Map<Eigen::Matrix4d> M(modelview);
        double x = pixel.x();
        double y = viewport[3] - pixel.y();
        double z = pixel.z();
        Eigen::Vector4d in, out;
        in <<
            2 * (x - viewport[0]) / viewport[2] - 1, // Map between (-1,1)
            2 * (y - viewport[1]) / viewport[3] - 1, // Map between (-1,1)
            2 *  z                              - 1, // Fed in as 0 or 1 which maps to (-1,1)
            1;
        out = (P * M).inverse() * in;
        if(out[3] == 0)
            throw std::runtime_error("MarkovModelViewer::pixels2World: Failed.");
        x = out[0] / out[3];
        y = out[1] / out[3];
        z = out[2] / out[3];
        return QVector3D(x, y, z);
    }
    
    QVector3D MarkovModelViewer::world2Pixels(QVector3D world, int *viewport, double *projection, double *modelview)
    {
        Eigen::Map<Eigen::Matrix4d> P(projection);
        Eigen::Map<Eigen::Matrix4d> M(modelview);
        Eigen::Vector4d A, B;
        A << world.x(), world.y(), world.z(), 1;
        B = (M * P) * A;
        if(B[3] == 0)
            throw std::runtime_error("MarkovModelViewer::world2Pixels: Failed.");
        B[0] /= B[3];
        B[1] /= B[3];
        B[2] /= B[3];
        double x = viewport[0] + ((B[0] + 1) * viewport[2]) / 2; // Map from (-1,1)
        double y = viewport[1] + ((B[1] + 1) * viewport[3]) / 2; // Map from (-1,1)
        double z = (1 + B[2]) / 2; // Map from (-1,1) to (0,1)
        return QVector3D(round(x), round(viewport[3] - y), z);
    }
    
} // MarkovModel
