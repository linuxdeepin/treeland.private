// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "ddeshelsurfacewindow.h"

#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "wayland");
    QApplication app(argc, argv);

    DDEShelSurfaceWindow window;
    window.setWindowTitle("test for super overlay surface");
    window.resize(600, 200);
    window.show();

    return app.exec();
}

#include "main.moc"
