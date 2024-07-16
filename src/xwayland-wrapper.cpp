#include "types/types.h"

#include <DBusCompositor1.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusUnixFileDescriptor>

using DBusCompositor1 = org::deepin::Compositor1;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    XWaylandTask task;

    task.waylandSocketFd = QDBusUnixFileDescriptor(qgetenv("WAYLAND_SOCKET").toInt());

    for (int i = 0; i < argc; ++i) {
        printf("argv[%d] = %s\n", i, argv[i]);

        if (argv[i][0] == ':' && argv[i][1] != '\0') {
            // Extract the number after ':'
            int number = atoi(argv[i] + 1); // Convert string to integer after ':'
            printf("Extracted number: %d\n", number);
            task.display = argv[i];
        }

        // Check for specific options and their arguments
        if (strcmp(argv[i], "-rootless") == 0) {
            // Handle -rootless option
            printf("Option: -rootless\n");
            task.arguments.append(" ");
            task.arguments.append(argv[i]);
        } else if (strcmp(argv[i], "-core") == 0) {
            // Handle -core option
            printf("Option: -core\n");
            task.arguments.append(" ");
            task.arguments.append(argv[i]);
        } else if (strcmp(argv[i], "-terminate") == 0) {
            // Handle -terminate option
            printf("Option: -terminate\n");
            task.arguments.append(" ");
            task.arguments.append(argv[i]);
        } else if (strcmp(argv[i], "-listenfd") == 0) {
            // Handle -listenfd option, expect the next argument to be the file descriptor number
            if (i + 1 < argc) {
                int listenfd = atoi(argv[i + 1]); // Convert string to integer
                printf("Option: -listenfd %d\n", listenfd);
                task.listenFds.append(QDBusUnixFileDescriptor(dup(listenfd)));
                i++; // Move to the next argument
            } else {
                printf("Error: Missing argument after -listenfd\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-displayfd") == 0) {
            // Handle -displayfd option, expect the next argument to be the file descriptor number
            if (i + 1 < argc) {
                int displayfd = atoi(argv[i + 1]); // Convert string to integer
                printf("Option: -displayfd %d\n", displayfd);
                task.displayFd = QDBusUnixFileDescriptor(dup(displayfd));
                i++; // Move to the next argument
            } else {
                printf("Error: Missing argument after -displayfd\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-wm") == 0) {
            // Handle -wm option, expect the next argument to be the file descriptor number
            if (i + 1 < argc) {
                int wmfd = atoi(argv[i + 1]); // Convert string to integer
                printf("Option: -wm %d\n", wmfd);
                task.wmFd = QDBusUnixFileDescriptor(dup(wmfd));
                i++; // Move to the next argument
            } else {
                printf("Error: Missing argument after -wm\n");
                return 1;
            }
        }
    }

    qDBusRegisterMetaType<XWaylandTask>();

    DBusCompositor1 c1("org.deepin.Compositor1",
                       "/org/deepin/Compositor1",
                       QDBusConnection::sessionBus());
    if (c1.isValid()) {
        c1.UpdateXWaylandTask(task);
    } else {
        return -1;
    }

    QDBusServiceWatcher *compositorWatcher =
        new QDBusServiceWatcher("org.deepin.Compositor1",
                                QDBusConnection::sessionBus(),
                                QDBusServiceWatcher::WatchForRegistration);

    QObject::connect(compositorWatcher,
                     &QDBusServiceWatcher::serviceRegistered,
                     qApp,
                     &QCoreApplication::quit);

    return app.exec();
}
