/*
 * Copyright (C) 2014 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 * Diego Sarmentero <diego.sarmentero@canonical.com>
 *
*/

#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QtQml>
#include <QHash>
#include <QList>
#include <QVariant>
#include <QVariantList>
#include "system_update.h"
#include "update.h"
#include <token.h>

#ifdef TESTS
#include "../../tests/plugins/system-update/fakeprocess.h"
#include "../../tests/plugins/system-update/fakenetwork.h"
#include "../../tests/plugins/system-update/fakessoservice.h"
#include "../../tests/plugins/system-update/fakesystemupdate.h"
#else
#include <ssoservice.h>
#include <QProcess>
#include "network/network.h"
#include "system_update.h"
#endif

using namespace UbuntuOne;

namespace UpdatePlugin {

class UpdateManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList model READ model NOTIFY modelChanged)
    Q_PROPERTY(int downloadMode READ downloadMode WRITE setDownloadMode
               NOTIFY downloadModeChanged)
    Q_PROPERTY(int currentBuildNumber READ currentBuildNumber)

Q_SIGNALS:
    void checkFinished();
    void modelChanged();
    void updatesNotFound();
    void credentialsNotFound();
    void updateAvailableFound(bool downloading);
    void errorFound();
    void downloadModeChanged();
    void systemUpdateDownloaded();
    void updateProcessFailed(QString message);
    void systemUpdateFailed(int consecutiveFailureCount, QString lastReason);
    
public:
    explicit UpdateManager(QObject *parent = 0);
    ~UpdateManager();

    Q_INVOKABLE void checkUpdates();
    Q_INVOKABLE void startDownload(const QString &packagename);
    Q_INVOKABLE void pauseDownload(const QString &packagename);
    Q_INVOKABLE void retryDownload(const QString &packagename);
    Q_INVOKABLE void applySystemUpdate() { m_systemUpdate.applyUpdate(); }

    QVariantList model() const { return m_model; }
    int downloadMode() { return m_systemUpdate.downloadMode(); }
    void setDownloadMode(int mode) { m_systemUpdate.setDownloadMode(mode); }
    int currentBuildNumber() { return m_systemUpdate.currentBuildNumber(); }

#ifdef TESTS
    // For testing purposes
    QHash<QString, Update*> get_apps() { return m_apps; }
    QVariantList get_model() { return m_model; }
    int get_downloadMode() { return m_downloadMode; }
    void set_token(Token& t) { m_token = t; }
    Token get_token() { return m_token; }
#endif

public Q_SLOTS:
    void registerSystemUpdate(const QString& packageName, Update *update);

private Q_SLOTS:
    void clickUpdateNotAvailable();
    void systemUpdateNotAvailable();
    void systemUpdatePaused(int value);
    void processOutput();
    void processUpdates();
    void downloadUrlObtained(const QString &packagename, const QString &url);
    void handleCredentialsFound(Token token);
    void clickTokenReceived(Update *app, const QString &clickToken);

private:
    bool m_systemCheckingUpdate;
    bool m_clickCheckingUpdate;
    int m_checkingUpdates;
    QHash<QString, Update*> m_apps;
    int m_downloadMode;
    QVariantList m_model;
    Token m_token;

#ifdef TESTS
    FakeNetwork m_network;
    FakeProcess m_process;
    FakeSsoService m_service;
    FakeSystemUpdate m_systemUpdate;
#else
    Network m_network;
    QProcess m_process;
    SSOService m_service;
    SystemUpdate m_systemUpdate;
#endif

    void checkForUpdates();
    QString getClickCommand();
    bool getCheckForCredentials();
    void reportCheckState();
    void updateNotAvailable();
};

}

#endif // UPDATEMANAGER_H