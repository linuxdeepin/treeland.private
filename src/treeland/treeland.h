// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <WServer>
#include <wsocket.h>

#include <QDBusContext>
#include <QDBusUnixFileDescriptor>
#include <QGuiApplication>
#include <QLocalSocket>
#include <QtWaylandCompositor/QWaylandCompositor>

#include <memory>

class QQmlApplicationEngine;
class Helper;

namespace TreeLand {

struct TreeLandAppContext
{
    QString socket;
    QString run;
};

class TreeLand
    : public QObject
    , protected QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(bool testMode READ testMode CONSTANT)
    Q_PROPERTY(bool debugMode READ debugMode CONSTANT FINAL)

public:
    explicit TreeLand(Helper *helper, TreeLandAppContext context);

    Q_INVOKABLE void retranslate() noexcept;

    bool testMode() const;

    bool debugMode() const;

Q_SIGNALS:
    void socketDisconnected();

public Q_SLOTS:
    bool ActivateWayland(QDBusUnixFileDescriptor fd);
    QString XWaylandName();

private Q_SLOTS:
    void connected();
    void disconnected();
    void readyRead();
    void error();

private:
    TreeLandAppContext m_context;
    QLocalSocket *m_socket{ nullptr };
    QLocalSocket *m_helperSocket{ nullptr };
    Helper *m_helper{ nullptr };
    QMap<QString, std::shared_ptr<Waylib::Server::WSocket>> m_userWaylandSocket;
    QMap<QString, std::shared_ptr<QDBusUnixFileDescriptor>> m_userDisplayFds;
};
} // namespace TreeLand
