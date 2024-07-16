#pragma once

#include <QDBusArgument>
#include <QDBusUnixFileDescriptor>
#include <QMetaType>

struct XWaylandTask
{
    QList<QDBusUnixFileDescriptor> listenFds;
    QDBusUnixFileDescriptor displayFd;
    QDBusUnixFileDescriptor wmFd;
    QDBusUnixFileDescriptor waylandSocketFd;
    QString display;
    QString arguments;
};

static QDBusArgument &operator<<(QDBusArgument &argument, const XWaylandTask &task)
{
    argument.beginStructure();
    argument << task.listenFds;
    argument << task.displayFd;
    argument << task.wmFd;
    argument << task.waylandSocketFd;
    argument << task.display;
    argument << task.arguments;
    argument.endStructure();
    return argument;
}

static const QDBusArgument &operator>>(const QDBusArgument &argument, XWaylandTask &task)
{
    argument.beginStructure();
    argument >> task.listenFds;
    argument >> task.displayFd;
    argument >> task.wmFd;
    argument >> task.waylandSocketFd;
    argument >> task.display;
    argument >> task.arguments;
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(XWaylandTask);
