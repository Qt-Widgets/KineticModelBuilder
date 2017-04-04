/* --------------------------------------------------------------------------------
 * QObject property editor UI.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __QObjectPropertyEditor_H__
#define __QObjectPropertyEditor_H__

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVariant>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#include <QDateTime>
#endif

namespace QObjectPropertyEditor {
    
    // Handle descendant properties such as "child.grandchild.property".
    QObject* descendant(QObject *object, const QByteArray &pathToDescendantObject);
    
    /* --------------------------------------------------------------------------------
     * Things that all QObject property models should be able to do.
     * -------------------------------------------------------------------------------- */
    class QAbstractPropertyModel : public QAbstractItemModel
    {
        Q_OBJECT
        
    public:
        QAbstractPropertyModel(QObject *parent = 0) : QAbstractItemModel(parent) {}
        
        virtual QObject* objectAtIndex(const QModelIndex &index) const = 0;
        virtual QByteArray propertyNameAtIndex(const QModelIndex &index) const = 0;
        const QMetaProperty metaPropertyAtIndex(const QModelIndex &index) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        Qt::ItemFlags flags(const QModelIndex &index) const;
    };
    
    /* --------------------------------------------------------------------------------
     * Property model for a single QObject.
     * -------------------------------------------------------------------------------- */
    class QObjectPropertyModel : public QAbstractPropertyModel
    {
        Q_OBJECT
        
    public:
        QObjectPropertyModel(QObject *parent = 0) : QAbstractPropertyModel(parent), _object(0) {}
        
        // Property getters.
        QObject* object() const { return _object; }
        QList<QByteArray> propertyNames() const { return _propertyNames; }
        QHash<QByteArray, QString> propertyHeaders() const { return _propertyHeaders; }
        
        // Property setters. !!! Remember to call beginResetModel() and endResetModel() around your model changes.
        void setObject(QObject *obj) { _object = obj; }
        void setPropertyNames(const QList<QByteArray> &names) { _propertyNames = names; }
        void setPropertyHeaders(const QHash<QByteArray, QString> &headers) { _propertyHeaders = headers; }
        
        // Model interface.
        QObject* objectAtIndex(const QModelIndex &index) const;
        QByteArray propertyNameAtIndex(const QModelIndex &index) const;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &index) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        
    protected:
        QObject *_object;
        QList<QByteArray> _propertyNames;
        QHash<QByteArray, QString> _propertyHeaders;
    };
    
    /* --------------------------------------------------------------------------------
     * Property model for a list of QObjects (same properties for each).
     * -------------------------------------------------------------------------------- */
    class QObjectListPropertyModel : public QAbstractPropertyModel
    {
        Q_OBJECT
        
    public:
        typedef QObject* (*ObjectCreatorFuncPtr)();
        
        QObjectListPropertyModel(QObject *parent = 0) : QAbstractPropertyModel(parent), _parentOfObjects(0), _objectCreator(0) {}
        
        // Property getters.
        QObjectList objects() const { return _objects; }
        QList<QByteArray> propertyNames() const { return _propertyNames; }
        QHash<QByteArray, QString> propertyHeaders() const { return _propertyHeaders; }
        QObject* parentOfObjects() const { return _parentOfObjects; }
        ObjectCreatorFuncPtr objectCreator() const { return _objectCreator; }
        
        // Property setters. !!! Remember to call beginResetModel() and endResetModel() around your model changes.
        void setObjects(const QObjectList &objects) { _objects = objects; }
        void setPropertyNames(const QList<QByteArray> &names) { _propertyNames = names; }
        void setPropertyHeaders(const QHash<QByteArray, QString> &headers) { _propertyHeaders = headers; }
        void setParentOfObjects(QObject *parent) { _parentOfObjects = parent; }
        void setObjectCreator(ObjectCreatorFuncPtr creator) { _objectCreator = creator; }
        
        // Model interface.
        QObject* objectAtIndex(const QModelIndex &index) const;
        QByteArray propertyNameAtIndex(const QModelIndex &index) const;
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        QModelIndex parent(const QModelIndex &index) const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
        bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
        bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow);
        void reorderChildObjectsToMatchRowOrder(int firstRow = 0);
        
    protected:
        QObjectList _objects;
        QList<QByteArray> _propertyNames;
        QHash<QByteArray, QString> _propertyHeaders;
        QObject *_parentOfObjects;
        ObjectCreatorFuncPtr _objectCreator;
    };
    
    /* --------------------------------------------------------------------------------
     * Property editor delegate.
     * -------------------------------------------------------------------------------- */
    class QObjectPropertyDelegate: public QStyledItemDelegate
    {
    public:
        QObjectPropertyDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}
        
        QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
        void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE;
        QString displayText(const QVariant &value, const QLocale &locale) const Q_DECL_OVERRIDE;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    
    protected:
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) Q_DECL_OVERRIDE;
    };
    
    /* --------------------------------------------------------------------------------
     * Editor for properties in a single QObject.
     * -------------------------------------------------------------------------------- */
    class QObjectPropertyEditor : public QTableView
    {
        Q_OBJECT
        
    public:
        QObjectPropertyEditor(QWidget *parent = 0);
    
    protected:
        QObjectPropertyDelegate _delegate;
    };
    
    /* --------------------------------------------------------------------------------
     * Editor for properties in a list of QObjects.
     * -------------------------------------------------------------------------------- */
    class QObjectListPropertyEditor : public QTableView
    {
        Q_OBJECT
        
    public:
        QObjectListPropertyEditor(QWidget *parent = 0);
        
    public slots:
        void horizontalHeaderContextMenu(QPoint pos);
        void verticalHeaderContextMenu(QPoint pos);
        void appendRow();
        void insertSelectedRows();
        void removeSelectedRows();
        void handleSectionMove(int logicalIndex, int oldVisualIndex, int newVisualIndex);
        
    protected:
        QObjectPropertyDelegate _delegate;
        
        void keyPressEvent(QKeyEvent *event);
    };
    
#ifdef DEBUG
    /* --------------------------------------------------------------------------------
     * QObject derived class with some properties.
     * -------------------------------------------------------------------------------- */
    class TestObject : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(MyEnum myEnum READ myEnum WRITE setMyEnum)
        Q_PROPERTY(MyEnum myReadOnlyEnum READ myEnum)
        Q_PROPERTY(bool myBool READ myBool WRITE setMyBool)
        Q_PROPERTY(bool myReadOnlyBool READ myBool)
        Q_PROPERTY(int myInt READ myInt WRITE setMyInt)
        Q_PROPERTY(int myReadOnlyInt READ myInt)
        Q_PROPERTY(float myFloat READ myFloat WRITE setMyFloat)
        Q_PROPERTY(float myReadOnlyFloat READ myFloat)
        Q_PROPERTY(double myDouble READ myDouble WRITE setMyDouble)
        Q_PROPERTY(double myReadOnlyDouble READ myDouble)
        Q_PROPERTY(QString myString READ myString WRITE setMyString)
        Q_PROPERTY(QString myReadOnlyString READ myString)
        Q_PROPERTY(QDateTime myDateTime READ myDateTime WRITE setMyDateTime)
        Q_PROPERTY(QDateTime myReadOnlyDateTime READ myDateTime)
        Q_PROPERTY(QSize mySize READ mySize WRITE setMySize)
        Q_PROPERTY(QSizeF mySizeF READ mySizeF WRITE setMySizeF)
        Q_PROPERTY(QPoint myPoint READ myPoint WRITE setMyPoint)
        Q_PROPERTY(QPointF myPointF READ myPointF WRITE setMyPointF)
        Q_PROPERTY(QRect myRect READ myRect WRITE setMyRect)
        Q_PROPERTY(QRectF myRectF READ myRectF WRITE setMyRectF)
        
    public:
        // Custom enum will be editable via a QComboBox so long as we tell Qt about it with Q_ENUMS().
        enum  MyEnum { A, B, C };
        Q_ENUMS(MyEnum)
        
        // Init.
        TestObject(const QString &name = "", QObject *parent = 0) :
        QObject(parent),
        _myEnum(B),
        _myBool(true),
        _myInt(82),
        _myFloat(3.14),
        _myDouble(3.14e-12),
        _myString("Hi-ya!"),
        _myDateTime(QDateTime::currentDateTime()),
        _mySize(2, 4),
        _mySizeF(3.1, 4.9),
        _myPoint(0, 1),
        _myPointF(0.05, 1.03),
        _myRect(0, 0, 3, 3),
        _myRectF(0.5, 0.5, 1.3, 3.1)
        {
            setObjectName(name);
        }
        
        // Property getters.
        MyEnum myEnum() const { return _myEnum; }
        bool myBool() const { return _myBool; }
        int myInt() const { return _myInt; }
        float myFloat() const { return _myFloat; }
        double myDouble() const { return _myDouble; }
        QString myString() const { return _myString; }
        QDateTime myDateTime() const { return _myDateTime; }
        QSize mySize() const { return _mySize; }
        QSizeF mySizeF() const { return _mySizeF; }
        QPoint myPoint() const { return _myPoint; }
        QPointF myPointF() const { return _myPointF; }
        QRect myRect() const { return _myRect; }
        QRectF myRectF() const { return _myRectF; }
        
        // Property setters.
        void setMyEnum(MyEnum myEnum) { _myEnum = myEnum; }
        void setMyBool(bool myBool) { _myBool = myBool; }
        void setMyInt(int myInt) { _myInt = myInt; }
        void setMyFloat(float myFloat) { _myFloat = myFloat; }
        void setMyDouble(double myDouble) { _myDouble = myDouble; }
        void setMyString(QString myString) { _myString = myString; }
        void setMyDateTime(QDateTime myDateTime) { _myDateTime = myDateTime; }
        void setMySize(QSize mySize) { _mySize = mySize; }
        void setMySizeF(QSizeF mySizeF) { _mySizeF = mySizeF; }
        void setMyPoint(QPoint myPoint) { _myPoint = myPoint; }
        void setMyPointF(QPointF myPointF) { _myPointF = myPointF; }
        void setMyRect(QRect myRect) { _myRect = myRect; }
        void setMyRectF(QRectF myRectF) { _myRectF = myRectF; }
        
    protected:
        MyEnum _myEnum;
        bool _myBool;
        int _myInt;
        double _myFloat;
        double _myDouble;
        QString _myString;
        QDateTime _myDateTime;
        QSize _mySize;
        QSizeF _mySizeF;
        QPoint _myPoint;
        QPointF _myPointF;
        QRect _myRect;
        QRectF _myRectF;
    };
    
    int testQObjectPropertyEditor(int argc, char **argv);
    int testQObjectListPropertyEditor(int argc, char **argv);
#endif
    
} // QObjectPropertyEditor

#endif
