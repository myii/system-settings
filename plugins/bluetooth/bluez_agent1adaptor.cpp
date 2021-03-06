/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -a bluez_agent1adaptor -c BluezAgent1Adaptor org.bluez.Agent1.xml
 *
 * qdbusxml2cpp is Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "bluez_agent1adaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class BluezAgent1Adaptor
 */

BluezAgent1Adaptor::BluezAgent1Adaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

BluezAgent1Adaptor::~BluezAgent1Adaptor()
{
    // destructor
}

void BluezAgent1Adaptor::AuthorizeService(const QDBusObjectPath &device, const QString &uuid)
{
    // handle method call org.bluez.Agent1.AuthorizeService
    QMetaObject::invokeMethod(parent(), "AuthorizeService", Q_ARG(QDBusObjectPath, device), Q_ARG(QString, uuid));
}

void BluezAgent1Adaptor::Cancel()
{
    // handle method call org.bluez.Agent1.Cancel
    QMetaObject::invokeMethod(parent(), "Cancel");
}

void BluezAgent1Adaptor::DisplayPasskey(const QDBusObjectPath &device, uint passkey, ushort entered)
{
    // handle method call org.bluez.Agent1.DisplayPasskey
    QMetaObject::invokeMethod(parent(), "DisplayPasskey", Q_ARG(QDBusObjectPath, device), Q_ARG(uint, passkey), Q_ARG(ushort, entered));
}

void BluezAgent1Adaptor::DisplayPinCode(const QDBusObjectPath &device, const QString &pincode)
{
    // handle method call org.bluez.Agent1.DisplayPinCode
    QMetaObject::invokeMethod(parent(), "DisplayPinCode", Q_ARG(QDBusObjectPath, device), Q_ARG(QString, pincode));
}

void BluezAgent1Adaptor::Release()
{
    // handle method call org.bluez.Agent1.Release
    QMetaObject::invokeMethod(parent(), "Release");
}

void BluezAgent1Adaptor::RequestAuthorization(const QDBusObjectPath &device)
{
    // handle method call org.bluez.Agent1.RequestAuthorization
    QMetaObject::invokeMethod(parent(), "RequestAuthorization", Q_ARG(QDBusObjectPath, device));
}

void BluezAgent1Adaptor::RequestConfirmation(const QDBusObjectPath &device, uint passkey)
{
    // handle method call org.bluez.Agent1.RequestConfirmation
    QMetaObject::invokeMethod(parent(), "RequestConfirmation", Q_ARG(QDBusObjectPath, device), Q_ARG(uint, passkey));
}

uint BluezAgent1Adaptor::RequestPasskey(const QDBusObjectPath &device)
{
    // handle method call org.bluez.Agent1.RequestPasskey
    uint passkey;
    QMetaObject::invokeMethod(parent(), "RequestPasskey", Q_RETURN_ARG(uint, passkey), Q_ARG(QDBusObjectPath, device));
    return passkey;
}

QString BluezAgent1Adaptor::RequestPinCode(const QDBusObjectPath &device)
{
    // handle method call org.bluez.Agent1.RequestPinCode
    QString pincode;
    QMetaObject::invokeMethod(parent(), "RequestPinCode", Q_RETURN_ARG(QString, pincode), Q_ARG(QDBusObjectPath, device));
    return pincode;
}

