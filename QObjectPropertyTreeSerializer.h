/* --------------------------------------------------------------------------------
 * Tools for serializing properties in a QObject tree.
 * - Serialize/Deserialize to/from a QVariantMap.
 * - Read/Write from/to a JSON file.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __QObjectPropertyTreeSerializer_H__
#define __QObjectPropertyTreeSerializer_H__

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

namespace QObjectPropertyTreeSerializer {

/* --------------------------------------------------------------------------------
 * Object factory for dynamic object creation during deserialization.
 * -------------------------------------------------------------------------------- */
class ObjectFactory
{
public:
    typedef QObject* (*ObjectCreatorFuncPtr)();
    typedef QMap<QByteArray, ObjectCreatorFuncPtr> ObjectCreatorMap;

public:
    bool hasCreator(const QByteArray &className) const { return _objectCreatorMap.contains(className); }
    void registerCreator(const QByteArray &className, ObjectCreatorFuncPtr creator) { _objectCreatorMap[className] = creator; }
    QObject* create(const QByteArray &className) const { return _objectCreatorMap.contains(className) ? (*_objectCreatorMap.value(className))() : 0; }

private:
    ObjectCreatorMap _objectCreatorMap;
};

/* --------------------------------------------------------------------------------
 * Serialize/Deserialize to/from a QVariantMap.
 * -------------------------------------------------------------------------------- */
QVariantMap serialize(const QObject *object, int childDepth = -1, bool includeReadOnlyProperties = true, bool includeObjectName = true);
void deserialize(QObject *object, const QVariantMap &data, ObjectFactory *factory = 0);

// Helper function used by serialize() and deserialize().
void addMappedData(QVariantMap &data, const QByteArray &key, const QVariant &value);

/* --------------------------------------------------------------------------------
 * Read/Write from/to JSON.
 * -------------------------------------------------------------------------------- */
bool readJson(QObject *object, const QString &filePath, ObjectFactory *factory = 0);
bool writeJson(QObject *object, const QString &filePath);

} // QObjectPropertyTreeSerializer

#endif
