// Copyright (C) 2024 rewine <luhongxu@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wserver.h>

#include <QQmlEngine>

struct wallpaper_color_manager_v1;
WAYLIB_SERVER_USE_NAMESPACE

class WallpaperColorV1
    : public QObject
    , public WAYLIB_SERVER_NAMESPACE::WServerInterface
{
    Q_OBJECT

public:
    explicit WallpaperColorV1(QObject *parent = nullptr);
    Q_INVOKABLE void updateWallpaperColor(const QString &output, bool isDarkType);
    QByteArrayView interfaceName() const override;

protected:
    void create(WServer *server) override;
    void destroy(WServer *server) override;
    wl_global *global() const override;

private:
    wallpaper_color_manager_v1 *m_handle{ nullptr };
};
