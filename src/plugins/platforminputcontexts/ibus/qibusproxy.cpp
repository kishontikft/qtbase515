/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -N -p qibusproxy -c QIBusProxy interfaces/org.freedesktop.IBus.xml
 *
 * qdbusxml2cpp is Copyright (C) 2015 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#include <QtDBus/qdbusextratypes.h>

#include "qibusproxy.h"

/*
 * Implementation of interface class QIBusProxy
 */

QIBusProxy::QIBusProxy(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
    this->connection().connect(service,
                               path,
                               this->interface(), // interface
                               QStringLiteral("GlobalEngineChanged"),
                               QStringList(),
                               QString(), // signature
                               this, SLOT(globalEngineChanged(QString)));
}

QIBusProxy::~QIBusProxy()
{
}

#ifdef QIBUS_GET_ADDRESS
QString QIBusProxy::getAddress()
{
    QDBusReply<QDBusVariant> reply = Address();
    QVariant variant = reply.value().variant();
    if (!variant.isValid())
        return QString();
    return variant.toString();
}
#endif

#ifdef QIBUS_GET_ENGINES
QList<QIBusEngineDesc> QIBusProxy::getEngines()
{
    QList<QIBusEngineDesc> engines;
    QDBusReply<QDBusVariant> reply = Engines();
    QVariant variant = reply.value().variant();
    if (!variant.isValid())
        return engines;
    const QDBusArgument argument = variant.value<QDBusArgument>();
    qCDebug(qtQpaInputMethodsSerialize) << "QIBusProxy::getEngines()" << argument.currentSignature();

    int i = 1;
    argument.beginMap();
    while (!argument.atEnd()) {
        QDBusVariant value;
        argument >> value;
        if (!value.variant().isValid()) {
            qWarning() << "Warning in QIBusProxy::getEngines():" << QString::asprintf("%dth variant is wrong", i);
            break;
        }
        const QDBusArgument desc_arg = value.variant().value<QDBusArgument>();

        QIBusEngineDesc desc;
        desc_arg >> desc;
        engines.append(desc);
        ++i;
    }
    argument.endMap();
    return engines;
}
#endif

QIBusEngineDesc QIBusProxy::getGlobalEngine()
{
    QIBusEngineDesc desc;
    QDBusReply<QDBusVariant> reply = GlobalEngine();
    QVariant variant = reply.value().variant();
    if (!variant.isValid())
        return desc;
    QVariant child = qvariant_cast<QDBusVariant>(variant).variant();
    if (!child.isValid())
        return desc;
    const QDBusArgument argument = qvariant_cast<QDBusArgument>(child);
    argument >> desc;
    return desc;
}

void QIBusProxy::globalEngineChanged(const QString &engine_name)
{
    emit GlobalEngineChanged(engine_name);
}

