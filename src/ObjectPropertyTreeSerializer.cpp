/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "ObjectPropertyTreeSerializer.h"
#include <stdexcept>
#include <QFile>
#include <QJsonDocument>
#include <QMetaObject>
#include <QMetaProperty>
#include <QTextStream>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace ObjectPropertyTreeSerializer {

QVariantMap serialize(const QObject *object, int childDepth, bool includeReadOnlyProperties, bool includeObjectName)
{
    QVariantMap data;
    if(!object)
        return data;
    // Properties.
    const QMetaObject *metaObject = object->metaObject();
    int propertyCount = metaObject->propertyCount();
    for(int i = 0; i < propertyCount; ++i) {
        QMetaProperty metaProperty = metaObject->property(i);
        if(metaProperty.isReadable() && (includeReadOnlyProperties || metaProperty.isWritable())) {
            QByteArray propertyName = QByteArray(metaProperty.name());
            if(includeObjectName || propertyName != "objectName") {
                QVariant propertyValue = object->property(propertyName.constData());
                addMappedData(data, propertyName, propertyValue);
            }
        }
    }
    foreach(const QByteArray &propertyName, object->dynamicPropertyNames()) {
        QVariant propertyValue = object->property(propertyName.constData());
        addMappedData(data, propertyName, propertyValue);
    }
    // Children.
    if(childDepth == -1 || childDepth > 0) {
        if(childDepth > 0)
            --childDepth;
        foreach(QObject *child, object->children()) {
            QByteArray className(child->metaObject()->className());
            addMappedData(data, className, serialize(child, childDepth, includeReadOnlyProperties, includeObjectName));
        }
    }
    return data;
}

void deserialize(QObject *object, const QVariantMap &data, ObjectFactory *factory)
{
    if(!object)
        return;
    for(QVariantMap::const_iterator i = data.constBegin(); i != data.constEnd(); ++i) {
        if(i.value().type() == QVariant::Map) {
            // Child object.
            QByteArray className = i.key().toUtf8();
            const QVariantMap &childData = i.value().toMap();
            bool childFound = false;
            if(childData.contains("objectName")) {
                // If objectName is specified for the child, find the first existing child with matching objectName and className.
                QObjectList children = object->findChildren<QObject*>(childData.value("objectName").toString());
                foreach(QObject *child, children) {
                    if(className == QByteArray(child->metaObject()->className())) {
                        deserialize(child, childData, factory);
                        childFound = true;
                        break;
                    }
                }
            } else {
                // If objectName is NOT specified for the child, find the first existing child with matching className.
                foreach(QObject *child, object->children()) {
                    if(className == QByteArray(child->metaObject()->className())) {
                        deserialize(child, childData, factory);
                        childFound = true;
                        break;
                    }
                }
            }
            // If we still havent found an existing child, attempt to create one dynamically.
            if(!childFound) {
                QObject *child = 0;
                if(className == QByteArray("QObject"))
                    child = new QObject;
                else if(factory && factory->hasCreator(className))
                    child = factory->create(className);
                if(child) {
                    child->setParent(object);
                    deserialize(child, childData, factory);
                }
            }
        } else if(i.value().type() == QVariant::List) {
            // List of child objects and/or properties.
            QByteArray className = i.key().toUtf8();
            const QVariantList &childDataList = i.value().toList();
            QObjectList existingChildrenWithClassNameAndObjectName;
            QObjectList existingChildrenWithClassName;
            foreach(QObject *child, object->children()) {
                if(className == QByteArray(child->metaObject()->className())) {
                    if(!child->objectName().isEmpty())
                        existingChildrenWithClassNameAndObjectName.append(child);
                    else
                        existingChildrenWithClassName.append(child);
                }
            }
            for(QVariantList::const_iterator j = childDataList.constBegin(); j != childDataList.constEnd(); ++j) {
                if(j->type() == QVariant::Map) {
                    // Child object.
                    const QVariantMap &childData = j->toMap();
                    bool childFound = false;
                    if(childData.contains("objectName")) {
                        // If objectName is specified for the child, find the first existing child with matching objectName and className.
                        foreach(QObject *child, existingChildrenWithClassNameAndObjectName) {
                            if(child->objectName() == childData.value("objectName").toString()) {
                                deserialize(child, childData, factory);
                                existingChildrenWithClassNameAndObjectName.removeOne(child);
                                childFound = true;
                                break;
                            }
                        }
                    }
                    if(!childFound) {
                        // If objectName is NOT specified for the child or we could NOT find an object with the same name,
                        // find the first existing child with matching className.
                        if(!existingChildrenWithClassName.isEmpty()) {
                            QObject *child = existingChildrenWithClassName.first();
                            deserialize(child, childData, factory);
                            existingChildrenWithClassName.removeOne(child);
                            childFound = true;
                        }
                    }
                    // If we still havent found an existing child, attempt to create one dynamically.
                    if(!childFound) {
                        QObject *child = 0;
                        if(className == QByteArray("QObject"))
                            child = new QObject;
                        else if(factory && factory->hasCreator(className))
                            child = factory->create(className);
                        if(child) {
                            child->setParent(object);
                            deserialize(child, childData, factory);
                        }
                    }
                } else {
                    // Property.
                    // Deserialize named property.
                    const QByteArray &propertyName = className;
                    const QVariant &propertyValue = *j;
                    object->setProperty(propertyName.constData(), propertyValue);
                }
            }
        } else {
            // Property.
            // Deserialize named property.
            QByteArray propertyName = i.key().toUtf8();
            const QVariant &propertyValue = i.value();
            object->setProperty(propertyName.constData(), propertyValue);
        }
    }
}

void addMappedData(QVariantMap &data, const QByteArray &key, const QVariant &value)
{
    if(data.contains(key)) {
        QVariant &existingData = data[key];
        if(existingData.type() == QVariant::List) {
            QVariantList values = existingData.toList();
            values.append(value);
            data[key] = values;
        } else {
            QVariantList values;
            values.append(existingData);
            values.append(value);
            data[key] = values;
        }
    } else {
        data[key] = value;
    }
}

bool readJson(QObject *object, const QString &filePath, ObjectFactory *factory)
{
    QFile file(filePath);
    if(!file.open(QIODevice::Text | QIODevice::ReadOnly))
        return false;
    QString buffer = file.readAll();
    file.close();
    QVariantMap data = QJsonDocument::fromJson(buffer.toUtf8()).toVariant().toMap();
    deserialize(object, data, factory);
    return true;
}

bool writeJson(QObject *object, const QString &filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::Text | QIODevice::WriteOnly))
        return false;
    QTextStream out(&file);
    QVariantMap data = serialize(object);
    out << QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented);
    file.close();
    return true;
}

} // ObjectPropertyTreeSerializer
