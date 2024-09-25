// Copyright (C) 2024 rewine <luhongxu@deepin.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef CUSTOMEXTENSION_H
#define CUSTOMEXTENSION_H

#include "qwayland-treeland-personalization-manager-v1.h"

#include <QtGui/QWindow>
#include <QtWaylandClient/QWaylandClientExtension>

QT_BEGIN_NAMESPACE

class PersonalizationManager
    : public QWaylandClientExtensionTemplate<PersonalizationManager>
    , public QtWayland::treeland_personalization_manager_v1
{
    Q_OBJECT
public:
    explicit PersonalizationManager();
};

class PersonalizationWindow
    : public QWaylandClientExtensionTemplate<PersonalizationWindow>
    , public QtWayland::treeland_personalization_window_context_v1
{
    Q_OBJECT
public:
    explicit PersonalizationWindow(struct ::treeland_personalization_window_context_v1 *object);
};

class PersonalizationWallpaper
    : public QWaylandClientExtensionTemplate<PersonalizationWallpaper>
    , public QtWayland::treeland_personalization_wallpaper_context_v1
{
    Q_OBJECT
public:
    explicit PersonalizationWallpaper(
        struct ::treeland_personalization_wallpaper_context_v1 *object);

protected:
    void treeland_personalization_wallpaper_context_v1_metadata(const QString &metadata) override;
};

class PersonalizationCursor
    : public QWaylandClientExtensionTemplate<PersonalizationCursor>
    , public QtWayland::treeland_personalization_cursor_context_v1
{
    Q_OBJECT
public:
    explicit PersonalizationCursor(struct ::treeland_personalization_cursor_context_v1 *object);

protected:
    void personalization_cursor_context_v1_verfity(int32_t success);
    void personalization_cursor_context_v1_theme(const QString &name);
    void personalization_cursor_context_v1_size(uint32_t size);
};

QT_END_NAMESPACE

#endif // CUSTOMEXTENSION_H
