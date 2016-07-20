/*
 * This file is part of system-settings
 *
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "helpers.h"

#include "click/manager.h"
#include "click/client_impl.h"
#include "click/manifest_impl.h"
#include "click/sso_impl.h"
#include "click/tokendownloader_factory_impl.h"

#include <ubuntu-app-launch.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>

// FIXME: need to do this better including #include "../../src/i18n.h"
// and linking to it
#include <libintl.h>
QString _(const char *text)
{
    return QString::fromUtf8(dgettext(0, text));
}

namespace UpdatePlugin
{
namespace Click
{
Manager::Manager(QObject *parent)
    : QObject(parent)
    , m_client(new Click::ClientImpl(this))
    , m_manifest(new Click::ManifestImpl(this))
    , m_sso(new Click::SSOImpl(this))
    , m_downloadFactory(new Click::TokenDownloaderFactoryImpl)
    , m_model(SystemUpdate::instance()->updates())
{
    init();
}

Manager::Manager(Click::Client *client,
                 Click::Manifest *manifest,
                 Click::SSO *sso,
                 Click::TokenDownloaderFactory *downloadFactory,
                 UpdateModel *model,
                 QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_manifest(manifest)
    , m_sso(sso)
    , m_downloadFactory(downloadFactory)
    , m_model(model)
{
    init();
}

void Manager::init()
{
    initClient();
    initManifest();
    initSSO();

    connect(this, SIGNAL(checkStarted()), this, SLOT(handleCheckStart()));
    connect(this, SIGNAL(checkCompleted()), this, SLOT(handleCheckStop()));
    connect(this, SIGNAL(checkFailed()), this, SLOT(handleCheckStop()));
    connect(this, SIGNAL(checkCanceled()), this, SLOT(handleCheckStop()));
}

Manager::~Manager()
{
    delete m_downloadFactory;
}

void Manager::initTokenDownloader(const Click::TokenDownloader *downloader)
{
    connect(downloader, SIGNAL(downloadSucceeded(QSharedPointer<Update>)),
            this, SLOT(handleTokenDownload(QSharedPointer<Update>)));
    connect(downloader, SIGNAL(downloadFailed(QSharedPointer<Update>)),
            this, SLOT(handleTokenDownloadFailure(QSharedPointer<Update>)));
    connect(this, SIGNAL(checkCanceled()), downloader, SLOT(cancel()));
}

void Manager::initClient()
{
    connect(m_client, SIGNAL(metadataRequestSucceeded(const QByteArray&)),
            this, SLOT(handleMetadataSuccess(const QByteArray&)));
    connect(m_client, SIGNAL(serverError()),
            this, SLOT(handleCommunicationErrors()));
    connect(m_client, SIGNAL(networkError()),
            this, SLOT(handleCommunicationErrors()));
    connect(m_client, SIGNAL(credentialError()),
            this, SLOT(handleCommunicationErrors()));

    // Redirect signals to consumers.
    connect(m_client, SIGNAL(networkError()),
            this, SIGNAL(networkError()));
    connect(m_client, SIGNAL(serverError()),
            this, SIGNAL(serverError()));
    connect(m_client, SIGNAL(credentialError()),
            this, SIGNAL(credentialError()));
}

void Manager::initManifest()
{
    connect(m_manifest, SIGNAL(requestSucceeded(const QJsonArray&)),
            this, SLOT(handleManifestSuccess(const QJsonArray&)));
    connect(m_manifest, SIGNAL(requestFailed()),
            this, SLOT(handleManifestFailure()));
}

void Manager::initSSO()
{
    connect(m_sso, SIGNAL(credentialsRequestSucceeded(const UbuntuOne::Token&)),
            this, SLOT(handleCredentialsFound(const UbuntuOne::Token&)));
    connect(m_sso, SIGNAL(credentialsRequestFailed()),
            this, SLOT(handleCredentialsFailed()));
}

void Manager::check()
{
    // Don't check for click updates if there are no credentials,
    // instead ask for credentials.
    if (!m_authToken.isValid() && !Helpers::isIgnoringCredentials()) {
        m_sso->requestCredentials();
        return;
    }

    Q_EMIT checkStarted();

    m_updates.clear();
    m_manifest->request();
}

void Manager::retry(const QString &identifier, const uint &revision)
{
    /* We'll simply ask for another click token if a click update failed
    to install. */
    QSharedPointer<Update> u = m_model->get(identifier, revision);
    if (u->identifier() == identifier && u->revision() == revision) {
        m_model->setAvailable(identifier, revision, true);

        Click::TokenDownloader* dl = m_downloadFactory->create(u);

        dl->setAuthToken(m_authToken);
        initTokenDownloader(dl);
        dl->download();
    }
}

// Tries to shut down all checks we might currently be doing.
void Manager::cancel()
{
    // Abort all click update update data objects.
    // foreach (const QString &name, m_updates.keys())m_updates.value(name)->cancel();
    m_client->cancel();
    Q_EMIT checkCanceled();
}

void Manager::launch(const QString &appId)
{
    if (!ubuntu_app_launch_start_application(appId.toLatin1().data(), nullptr)) {
        qWarning() << Q_FUNC_INFO << "Could not launch app" << appId;
    }
}

void Manager::handleManifestSuccess(const QJsonArray &manifest)
{
    // Nothing to do.
    if (manifest.size() == 0) {
        Q_EMIT checkCompleted();
        return;
    }

    int i;
    for (i = 0; i < manifest.size(); i++) {
        QSharedPointer<Update> update = QSharedPointer<Update>(new Update);

        QJsonObject object = manifest.at(i).toObject();
        update->setIdentifier(object.value("name").toString());
        update->setTitle(object.value("title").toString());
        update->setLocalVersion(object.value("version").toString());
        update->setKind(Update::Kind::KindClick);

        /* We'll also need the package name, which will be the first key inside
        the “hooks” key. */
        if (object.contains("hooks") && object.value("hooks").isObject()) {
            QJsonObject hooks = object.value("hooks").toObject();
            if (!hooks.isEmpty()) {
                update->setPackageName(hooks.keys()[0]);
            }
        }

        QStringList command;
        command << Helpers::whichPkcon() << "-p" << "install-local" << "$file";
        update->setCommand(command);
        m_updates.insert(update->identifier(), update);
    }
    requestMetadata();
}

void Manager::handleManifestFailure()
{
    if (m_checking)
        Q_EMIT checkFailed();
}

void Manager::handleTokenDownload(QSharedPointer<Update> update)
{
    Click::TokenDownloader* dl = qobject_cast<Click::TokenDownloader*>(QObject::sender());
    dl->disconnect();
    // Token reported as downloaded, but empty.
    if (update->token().isEmpty()) {
        m_updates.remove(update->identifier());
    }

    // Assume the original update was changed during the download of token.
    QSharedPointer<Update> freshUpdate = m_model->get(update->identifier(),
                                                      update->revision());
    if (freshUpdate) {
        freshUpdate->setToken(update->token());
        m_model->add(freshUpdate);
    } else {
        m_model->add(update);
    }

    dl->deleteLater();
    completionCheck();
}

void Manager::completionCheck()
{
    // Check if all tokens are fetched.
    foreach (const QString &name, m_updates.keys()){
        if (m_updates.value(name)->token().isEmpty()) {
            return; // Not done.
        }
    }

    // All updates had signed download urls, so we're done.
    Q_EMIT checkCompleted();
}

void Manager::handleTokenDownloadFailure(QSharedPointer<Update> update)
{
    Click::TokenDownloader* dl = qobject_cast<Click::TokenDownloader*>(QObject::sender());
    dl->disconnect();

    // Assume the original update was changed during the download of token.
    QSharedPointer<Update> freshUpdate = m_model->get(update->identifier(),
                                                      update->revision());
    if (freshUpdate) {
        freshUpdate->setToken("");
        m_model->add(freshUpdate);
    } else {
        update->setToken("");
        m_model->add(update);
    }

    // We're done with it.
    m_updates.remove(update->identifier());
    dl->deleteLater();
    completionCheck();
}

void Manager::handleCredentialsFound(const UbuntuOne::Token &token)
{
    m_authToken = token;

    if (!m_authToken.isValid() && !Helpers::isIgnoringCredentials()) {
        handleCredentialsFailed();
        return;
    }

    setAuthenticated(true);

    cancel();
    check();
}

void Manager::handleCredentialsFailed()
{
    m_sso->invalidateCredentials();
    m_authToken = UbuntuOne::Token();

    cancel();

    // We've invalidated the token, and the user is now not authenticated.
    setAuthenticated(false);
}

void Manager::handleCommunicationErrors()
{
    if (m_checking)
        Q_EMIT checkFailed();
}

void Manager::requestMetadata()
{
    QList<QString> packages;
    Q_FOREACH(const QString &name, m_updates.keys()) {
        packages.append(name);
    }

    QString urlApps = Helpers::clickMetadataUrl();

    QString authHeader;
    if (!Helpers::isIgnoringCredentials()) {
        authHeader = m_authToken.signUrl(
            urlApps, QStringLiteral("POST"), true
        );
    }
    QUrl url(urlApps);
    url.setQuery(authHeader);


    m_client->requestMetadata(url, packages);
}

void Manager::handleMetadataSuccess(const QByteArray &metadata)
{
    QJsonParseError *jsonError = new QJsonParseError;
    auto document = QJsonDocument::fromJson(metadata, jsonError);

    if (document.isArray()) {
        parseMetadata(document.array());
    } else {
        qCritical() << "Server click metadata was not an array.";
        handleCommunicationErrors();
    }

    if (jsonError->error != QJsonParseError::NoError) {
        qCritical() << "Server click metadata parsing failed:" << jsonError->errorString();
        handleCommunicationErrors();
    }

    delete jsonError;
}

void Manager::parseMetadata(const QJsonArray &array)
{
    for (int i = 0; i < array.size(); i++) {
        auto object = array.at(i).toObject();
        auto name = object["name"].toString();
        auto version = object["version"].toString();
        auto icon_url = object["icon_url"].toString();
        auto url = object["download_url"].toString();
        auto download_sha512 = object["download_sha512"].toString();
        auto changelog = object["changelog"].toString();
        auto size = object["binary_filesize"].toInt();
        auto title = object["title"].toString();
        auto revision = object["revision"].toInt();
        if (m_updates.contains(name)) {
            QSharedPointer<Update> update = m_updates.value(name);
            update->setRemoteVersion(version);
            if (update->isUpdateRequired()) {
                update->setIconUrl(icon_url);
                update->setDownloadUrl(url);
                update->setBinaryFilesize(size);
                update->setDownloadHash(download_sha512);
                update->setChangelog(changelog);
                update->setTitle(title);
                update->setRevision(revision);
                update->setState(Update::State::StateAvailable);

                Click::TokenDownloader* dl = m_downloadFactory->create(update);
                dl->setAuthToken(m_authToken);
                initTokenDownloader(dl);
                dl->download();
            } else {
                // Update not required, let's remove it.
                m_updates.remove(update->identifier());
                completionCheck();
            }
        }
    }

    // Prune m_updates, removing those without necessary update data. These are
    // either locally installed clicks, or in some other way not in the meta data.
    foreach (const QString &name, m_updates.keys()) {
        if (m_updates.value(name)->remoteVersion().isEmpty())
            m_updates.remove(name);
    }
    completionCheck();
}

bool Manager::authenticated() const
{
    return m_authenticated || Helpers::isIgnoringCredentials();
}

bool Manager::checkingForUpdates() const
{
    return m_checking;
}

void Manager::setAuthenticated(const bool authenticated)
{
    if (authenticated != m_authenticated) {
        m_authenticated = authenticated;
        Q_EMIT authenticatedChanged();
    }
}

void Manager::setCheckingForUpdates(const bool checking)
{
    if (checking != m_checking) {
        m_checking = checking;
        Q_EMIT checkingForUpdatesChanged();
    }
}
} // Click
} // UpdatePlugin
