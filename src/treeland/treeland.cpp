// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "treeland.h"

#include "cmdline.h"
#include "compositor1adaptor.h"
#include "helper.h"
#include "shellhandler.h"

#include <Messages.h>
#include <SignalHandler.h>
#include <SocketWriter.h>

#include <WCursor>
#include <WSeat>
#include <WSurface>
#include <wordexp.h>
#include <woutputrenderwindow.h>
#include <wrenderhelper.h>
#include <wsocket.h>
#include <wxwayland.h>

#include <qwbackend.h>
#include <qwcompositor.h>
#include <qwdatacontrolv1.h>
#include <qwdisplay.h>
#include <qwlogging.h>
#include <qwoutput.h>
#include <qwxwayland.h>

#include <DLog>

#include <QDebug>
#include <QEventLoop>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickView>
#include <QTimer>
#include <QtLogging>

#include <pwd.h>
#include <sys/socket.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(treelandMain, "treeland.dbus", QtDebugMsg);

using namespace DDM;
DCORE_USE_NAMESPACE;

QString unescape(const QString &str) noexcept
{
    QString unescapedStr;
    for (qsizetype i = 0; i < str.size(); ++i) {
        auto c = str.at(i);
        if (c != '\\') {
            unescapedStr.append(c);
            continue;
        }

        switch (str.at(i + 1).toLatin1()) {
        default:
            unescapedStr.append(c);
            break;
        case 'n':
            unescapedStr.append('\n');
            ++i;
            break;
        case 't':
            unescapedStr.append('\t');
            ++i;
            break;
        case 'r':
            unescapedStr.append('\r');
            ++i;
            break;
        case '\\':
            unescapedStr.append('\\');
            ++i;
            break;
        case ';':
            unescapedStr.append(';');
            ++i;
            break;
        case 's': {
            unescapedStr.append(R"(\ )");
            ++i;
        } break;
        }
    }

    return unescapedStr;
}

std::optional<QStringList> unescapeExecArgs(const QString &str) noexcept
{
    auto unescapedStr = unescape(str);
    if (unescapedStr.isEmpty()) {
        qCWarning(treelandMain) << "unescape Exec failed.";
        return std::nullopt;
    }

    auto deleter = [](wordexp_t *word) {
        wordfree(word);
        delete word;
    };
    std::unique_ptr<wordexp_t, decltype(deleter)> words{ new (std::nothrow)
                                                             wordexp_t{ 0, nullptr, 0 },
                                                         deleter };

    if (auto ret = wordexp(unescapedStr.toLocal8Bit(), words.get(), WRDE_SHOWERR); ret != 0) {
        QString errMessage;
        switch (ret) {
        case WRDE_BADCHAR:
            errMessage = "BADCHAR";
            break;
        case WRDE_BADVAL:
            errMessage = "BADVAL";
            break;
        case WRDE_CMDSUB:
            errMessage = "CMDSUB";
            break;
        case WRDE_NOSPACE:
            errMessage = "NOSPACE";
            break;
        case WRDE_SYNTAX:
            errMessage = "SYNTAX";
            break;
        default:
            errMessage = "unknown";
        }
        qCWarning(treelandMain) << "wordexp error: " << errMessage;
        return std::nullopt;
    }

    QStringList execList;
    for (std::size_t i = 0; i < words->we_wordc; ++i) {
        execList.emplace_back(words->we_wordv[i]);
    }

    return execList;
}

namespace TreeLand {
TreeLand::TreeLand(Helper *helper)
    : QObject()
    , m_helper(helper)
{
    m_socket = new QLocalSocket(this);

    connect(m_socket, &QLocalSocket::connected, this, &TreeLand::connected);
    connect(m_socket, &QLocalSocket::disconnected, this, &TreeLand::disconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &TreeLand::readyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &TreeLand::error);

    if (CmdLine::ref().socket().has_value()) {
        new DDM::SignalHandler;
        Q_ASSERT(m_helper);
        auto connectToServer = [this] {
            m_socket->connectToServer(CmdLine::ref().socket().value());
        };

        connect(m_helper, &Helper::socketFileChanged, this, connectToServer);

        WSocket *defaultSocket = m_helper->defaultWaylandSocket();
        if (defaultSocket && defaultSocket->isValid()) {
            connectToServer();
        }
    } else {
        m_helper->setCurrentUserId(getuid());
    }

    if (CmdLine::ref().run().has_value()) {
        auto exec = [runCmd = CmdLine::ref().run().value(), this] {
            qCInfo(treelandMain) << "run cmd:" << runCmd;
            if (auto cmdline = unescapeExecArgs(runCmd); cmdline) {
                auto cmdArgs = cmdline.value();

                auto envs = QProcessEnvironment::systemEnvironment();
                envs.insert("WAYLAND_DISPLAY", m_helper->defaultWaylandSocket()->fullServerName());
                envs.insert("DISPLAY", m_helper->defaultXWaylandSocket()->displayName());
                envs.insert("DDE_CURRENT_COMPOSITOR", "TreeLand");
                envs.insert("QT_IM_MODULE", "fcitx");

                QProcess process;
                process.setProgram(cmdArgs.constFirst());
                process.setArguments(cmdArgs.mid(1));
                process.setProcessEnvironment(envs);
                process.setStandardInputFile(QProcess::nullDevice());
                process.setStandardOutputFile(QProcess::nullDevice());
                process.setStandardErrorFile(QProcess::nullDevice());
                process.startDetached();
            }
        };
        auto con =
            connect(m_helper, &Helper::socketFileChanged, this, exec, Qt::SingleShotConnection);

        WSocket *defaultSocket = m_helper->defaultWaylandSocket();
        if (defaultSocket && defaultSocket->isValid()) {
            QObject::disconnect(con);
            exec();
        }
    }

    new Compositor1Adaptor(this);

    // init dbus after QML engine loaded.
    QDBusConnection::systemBus().registerService("org.deepin.Compositor1");
    QDBusConnection::systemBus().registerObject("/org/deepin/Compositor1", this);

    QDBusConnection::sessionBus().registerService("org.deepin.Compositor1");
    QDBusConnection::sessionBus().registerObject("/org/deepin/Compositor1", this);
}

bool TreeLand::testMode() const
{
    return !CmdLine::ref().socket().has_value();
}

bool TreeLand::debugMode() const
{
#ifdef QT_DEBUG
    return true;
#endif

    return qEnvironmentVariableIsSet("TREELAND_ENABLE_DEBUG");
}

void TreeLand::connected()
{
    // log connection
    qCDebug(treelandMain) << "Connected to the daemon.";

    // send connected message
    DDM::SocketWriter(m_socket) << quint32(DDM::GreeterMessages::Connect);
}

void TreeLand::retranslate() noexcept
{
    // m_engine->retranslate();
}

void TreeLand::disconnected()
{
    // log disconnection
    qCDebug(treelandMain) << "Disconnected from the daemon.";

    Q_EMIT socketDisconnected();

    qCDebug(treelandMain) << "Display Manager is closed socket connect, quiting treeland.";
    qApp->exit();
}

void TreeLand::error()
{
    qCritical() << "Socket error: " << m_socket->errorString();
}

void TreeLand::readyRead()
{
    // input stream
    QDataStream input(m_socket);

    while (input.device()->bytesAvailable()) {
        // read message
        quint32 message;
        input >> message;

        switch (DDM::DaemonMessages(message)) {
        case DDM::DaemonMessages::Capabilities: {
            // log message
            qCDebug(treelandMain) << "Message received from daemon: Capabilities";
        } break;
        case DDM::DaemonMessages::LoginSucceeded:
        case DDM::DaemonMessages::UserActivateMessage: {
            QString user;
            input >> user;

            for (auto key : m_userWaylandSocket.keys()) {
                m_userWaylandSocket[key]->setEnabled(key == user);
            }

            struct passwd *pwd = getpwnam(user.toUtf8());
            m_helper->setCurrentUserId(pwd->pw_uid);
        } break;
        case DDM::DaemonMessages::SwitchToGreeter: {
            // Q_EMIT m_helper->greeterVisibleChanged();
        } break;
        default:
            break;
        }
    }
}

bool TreeLand::ActivateWayland(QDBusUnixFileDescriptor _fd)
{
    if (!_fd.isValid()) {
        return false;
    }

    auto fd = std::make_shared<QDBusUnixFileDescriptor>();
    fd->swap(_fd);

    auto uid = connection().interface()->serviceUid(message().service());
    struct passwd *pw;
    pw = getpwuid(uid);
    QString user{ pw->pw_name };

    auto socket = std::make_shared<WSocket>(true);
    socket->create(fd->fileDescriptor(), false);

    socket->setEnabled(m_helper->currentUserId() == pw->pw_uid);

    m_helper->addSocket(socket.get());

    m_userWaylandSocket[user] = socket;
    m_userDisplayFds[user] = fd;

    connect(connection().interface(),
            &QDBusConnectionInterface::serviceUnregistered,
            socket.get(),
            [this, user] {
                m_userWaylandSocket.remove(user);
                m_userDisplayFds.remove(user);
            });

    return true;
}

QString TreeLand::XWaylandName()
{
    setDelayedReply(true);

    auto uid = connection().interface()->serviceUid(message().service());
    struct passwd *pw;
    pw = getpwuid(uid);
    QString user{ pw->pw_name };

    auto *xwayland = m_helper->createXWayland();
    const QString &display = xwayland->displayName();

    auto m = message();
    auto conn = connection();

    QProcess *process = new QProcess(this);
    connect(process, &QProcess::finished, [process, m, conn, user, display] {
        if (process->exitCode() != 0) {
            qCWarning(treelandMain)
                << "xhost command failed with exit code" << process->exitCode()
                << process->readAllStandardOutput() << process->readAllStandardError();
            auto reply =
                m.createErrorReply(QDBusError::InternalError, "Failed to set xhost permissions");
            conn.send(reply);
        } else {
            qCDebug(treelandMain) << process->exitCode() << " " << process->readAllStandardOutput()
                                  << process->readAllStandardError();
            qCDebug(treelandMain)
                << QString("user %1 got xwayland display %2.").arg(user).arg(display);
            auto reply = m.createReply(display);
            conn.send(reply);
        }
        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, [this, m, conn] {
        auto reply =
            m.createErrorReply(QDBusError::InternalError, "Failed to set xhost permissions");
        conn.send(reply);
    });

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("DISPLAY", display);
    process->setProcessEnvironment(env);
    process->setProgram("xhost");
    process->setArguments({ QString("+si:localuser:%1").arg(user) });
    process->start();

    return {};
}

} // namespace TreeLand
