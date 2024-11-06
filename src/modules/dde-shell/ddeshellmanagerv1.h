// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "impl/ddeshellmanagerv1impl.h"

#include <wserver.h>
#include <wsurfaceitem.h>

#include <QObject>
#include <QQmlEngine>
#include <QRegion>

WAYLIB_SERVER_USE_NAMESPACE

class DDEShellManagerV1;

class DDEShellAttached : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    DDEShellAttached(QQuickItem *target, QObject *parent = nullptr);

protected:
    QQuickItem *m_target;
};

class WindowOverlapChecker : public DDEShellAttached
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool overlapped READ overlapped WRITE setOverlapped NOTIFY overlappedChanged)

public:
    WindowOverlapChecker(QQuickItem *target, QObject *parent = nullptr);
    ~WindowOverlapChecker();

    inline bool overlapped() const
    {
        return m_overlapped;
    }

Q_SIGNALS:
    void overlappedChanged();

private:
    void setOverlapped(bool overlapped);

    bool m_overlapped{ false };
    QRect m_lastRect;

    friend class DDEShellManagerV1;
    inline static QRegion region;
};

class DDEShellHelper : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DDEShell)
    QML_UNCREATABLE("Only use for the attached.")
    QML_ATTACHED(DDEShellAttached)

public:
    using QObject::QObject;
    ~DDEShellHelper() override = default;

    static DDEShellAttached *qmlAttachedProperties(QObject *target);
};

class DDEShellManagerV1
    : public QObject
    , public WServerInterface
{
    Q_OBJECT
public:
    explicit DDEShellManagerV1(QObject *parent = nullptr);
    ~DDEShellManagerV1() override = default;

    void checkRegionalConflict(const QRegion &region);
    void sendActiveIn(uint32_t reason, const WSeat *seat);
    void sendActiveOut(uint32_t reason, const WSeat *seat);
    void sendStartDrag(const WSeat *seat);
    void sendDrop(const WSeat *seat);

    bool isDdeShellSurface(const WSurface *surface);
    treeland_dde_shell_surface *ddeShellSurfaceFromWSurface(const WSurface *surface) const;

Q_SIGNALS:
    void toggleMultitaskview();
    void requestPickWindow(treeland_window_picker_v1 *picker);

protected:
    void create(WServer *server) override;
    void destroy(WServer *server) override;
    wl_global *global() const override;
    QByteArrayView interfaceName() const override;

private:
    treeland_dde_shell_manager_v1 *m_manager = nullptr;
    QMap<treeland_window_overlap_checker *, QRect> m_conflictList;
};
