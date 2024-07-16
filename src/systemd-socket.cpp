// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "types/types.h"

#include <systemd/sd-daemon.h>

#include <DBusCompositor1.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QTimer>
#include <QtEnvironmentVariables>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using DBusCompositor1 = org::deepin::Compositor1;

void makeFdInheritable(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        qWarning() << "fcntl(F_GETFD) failed";
        return;
    }
    flags &= ~FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) == -1) {
        qWarning() << "fcntl(F_SETFD) failed";
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("TreeLand socket helper");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption typeOption(QStringList() << "t" << "type", "xwayland", "wayland");
    parser.addOption(typeOption);

    parser.process(app);

    const QString &type = parser.value(typeOption);

    if (sd_listen_fds(0) > 1) {
        fprintf(stderr, "Too many file descriptors received.\n");
        exit(1);
    }

    QDBusUnixFileDescriptor unixFileDescriptor(SD_LISTEN_FDS_START);

    qDBusRegisterMetaType<XWaylandTask>();

    auto active = [unixFileDescriptor, type](const QDBusConnection &connection) {
        auto activateFd = [unixFileDescriptor, type, connection] {
            DBusCompositor1 compositor("org.deepin.Compositor1",
                                       "/org/deepin/Compositor1",
                                       connection);

            if (compositor.isValid()) {
                if (type == "wayland") {
                    compositor.ActivateWayland(unixFileDescriptor);
                } else if (type == "xwayland") {
                    qDebug() << "running on xwayland";
                    QDBusPendingCallWatcher *watcher =
                        new QDBusPendingCallWatcher(compositor.ActivateXWayland());
                    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, [watcher] {
                        QDBusPendingReply<XWaylandTask> reply = *watcher;
                        if (reply.isError()) {
                            qWarning() << reply.error().message();
                        } else {
                            auto task = reply.value();
                            qDebug() << "接收到的 fd " << task.displayFd.fileDescriptor();

                            makeFdInheritable(task.displayFd.fileDescriptor());
                            makeFdInheritable(task.wmFd.fileDescriptor());
                            makeFdInheritable(task.waylandSocketFd.fileDescriptor());
                            for (auto fd : task.listenFds) {
                                makeFdInheritable(fd.fileDescriptor());
                            }

                            QProcess *process = new QProcess;
                            process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
                            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                            env.insert("WAYLAND_SOCKET",
                                       QString::number(task.waylandSocketFd.fileDescriptor()));
                            process->setProcessEnvironment(env);
                            process->setProgram("Xwayland");
                            process->setArguments(
                                QStringList() << QString("%1").arg(task.display) << "-rootless"
                                              << "-core" << "-terminate" <<
                                [task] {
                                    QStringList list;
                                    for (auto f : task.listenFds) {
                                        list << QString("-listenfd")
                                             << QString::number(f.fileDescriptor());
                                    }
                                    return list;
                                }() << QString("-displayfd")
                                              << QString::number(task.displayFd.fileDescriptor())
                                              << QString("-wm")
                                              << QString::number(task.wmFd.fileDescriptor()));
                            QObject::connect(process,
                                             &QProcess::readyReadStandardOutput,
                                             process,
                                             [=]() {
                                                 qDebug() << process->readAllStandardOutput();
                                             });
                            QObject::connect(process,
                                             &QProcess::readyReadStandardError,
                                             process,
                                             [=]() {
                                                 qDebug() << process->readAllStandardOutput();
                                             });
                            process->start();
                            process->waitForStarted();
                            qDebug() << process->exitStatus() << process->exitCode();
                            qDebug() << process->program();
                            qDebug() << process->arguments();
                            qDebug() << process->environment();

                            QDBusInterface systemd("org.freedesktop.systemd1",
                                                   "/org/freedesktop/systemd1",
                                                   "org.freedesktop.systemd1.Manager",
                                                   QDBusConnection::sessionBus());
                            systemd.call("SetEnvironment",
                                         QStringList() << QString("DISPLAY=%1").arg(task.display));
                        }
                        watcher->deleteLater();
                    });
                }
            }
        };

        QDBusServiceWatcher *compositorWatcher =
            new QDBusServiceWatcher("org.deepin.Compositor1",
                                    connection,
                                    QDBusServiceWatcher::WatchForRegistration);

        QObject::connect(compositorWatcher,
                         &QDBusServiceWatcher::serviceRegistered,
                         compositorWatcher,
                         activateFd);

        activateFd();
    };

    active(QDBusConnection::sessionBus());
    active(QDBusConnection::systemBus());

    sd_notify(0, "READY=1");

    return app.exec();
}
