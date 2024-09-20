// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "capture.h"

#include <QDir>
#include <QGuiApplication>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    TreeLandCaptureManager manager;
    QObject::connect(&manager, &TreeLandCaptureManager::activeChanged, &manager, [&app, &manager] {
        auto captureContext = manager.getContext();
        if (!captureContext) {
            app.exit(-1);
        }
        captureContext->selectSource(0x1 | 0x2 | 0x4, true, false, nullptr);
        QEventLoop loop;
        QObject::connect(captureContext,
                         &TreeLandCaptureContext::sourceReady,
                         &loop,
                         &QEventLoop::quit);
        loop.exec();
        auto frame = captureContext->frame();
        QImage result;
        QObject::connect(frame, &TreeLandCaptureFrame::ready, &app, [&result, &loop](QImage image) {
            result = image;
            loop.quit();
        });
        QObject::connect(frame, &TreeLandCaptureFrame::failed, &app, [&loop] {
            loop.quit();
        });
        loop.exec();
        if (result.isNull())
            app.exit(-1);
        ;
        auto saveBasePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        QDir saveBaseDir(saveBasePath);
        if (!saveBaseDir.exists())
            app.exit(-1);
        ;
        QString picName = "portal screenshot - " + QDateTime::currentDateTime().toString() + ".png";
        if (result.save(saveBaseDir.absoluteFilePath(picName), "PNG")) {
            qDebug() << saveBaseDir.absoluteFilePath(picName);
            app.quit();
        } else {
            app.exit(-1);
            ;
        }
    });
    return app.exec();
}
