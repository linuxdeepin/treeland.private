// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "impl/ddeshellv1.h"

#include <wserver.h>
#include <wsurfaceitem.h>

#include <QObject>
#include <QQmlEngine>

WAYLIB_SERVER_USE_NAMESPACE

class DDEShellV1;

class DDEShellAttached : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    DDEShellAttached(WSurfaceItem *target, QObject *parent = nullptr);

protected:
    WSurfaceItem *m_target;
};

class WindowOverlapChecker : public DDEShellAttached
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool overlapped READ overlapped WRITE setOverlapped NOTIFY overlappedChanged)

public:
    WindowOverlapChecker(WSurfaceItem *target, QObject *parent = nullptr);

    inline bool overlapped() const { return m_overlapped; }

Q_SIGNALS:
    void overlappedChanged();

private:
    void setOverlapped(bool overlapped);

    bool m_overlapped{ false };
};

class DDEShell : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DDEShell)
    QML_UNCREATABLE("Only use for the attached.")
    QML_ATTACHED(DDEShellAttached)

public:
    using QObject::QObject;
    ~DDEShell() override = default;

    static DDEShellAttached *qmlAttachedProperties(QObject *target);
};

class DDEShellV1
    : public QObject
    , public WServerInterface
{
    Q_OBJECT
    Q_PROPERTY(int shellLayerCount READ shellLayerCount)
public:
    explicit DDEShellV1(QObject *parent = nullptr);
    ~DDEShellV1() override = default;

    int shellLayerCount() { return m_shellLayerCount; }

    void checkRegionalConflict(WSurfaceItem *target);
    void sendActiveIn(uint32_t reason, QPointer<WSeat> seat);
    void sendActiveOut(uint32_t reason, QPointer<WSeat> seat);

    Q_INVOKABLE bool isDdeShellSurface(WSurface *surface);
    Q_INVOKABLE bool isSurfacePosInitialized(WSurface *surface);
    Q_INVOKABLE bool isSurfaceLayerInitialized(WSurface *surface);
    Q_INVOKABLE QPoint surfacePos(WSurface *surface) const;
    Q_INVOKABLE int surfaceLayerIndex(WSurface *surface) const;
    Q_INVOKABLE bool isSurfaceYOffsetInitialized(WSurface *surface);
    Q_INVOKABLE int32_t surfaceLayerYOffset(WSurface *surface) const;
    Q_INVOKABLE bool isSurfaceSkipSwitcherInitialized(WSurface *surface);
    Q_INVOKABLE bool isSurfaceSkipDockPreViewInitialized(WSurface *surface);
    Q_INVOKABLE bool isSurfaceSkipMutiTaskViewInitialized(WSurface *surface);

Q_SIGNALS:
    void surfacePositionChanged(treeland_dde_shell_surface *handle, QPoint pos);
    void surfaceLayerChanged(treeland_dde_shell_surface *handle, treeland_dde_shell_surface::Layer layer);
    void surfaceYOffsetChanged(treeland_dde_shell_surface *handle, int32_t offset);
    void surfaceSkipSwitcherChanged(treeland_dde_shell_surface *handle, bool skip);
    void surfaceSkipDockPreViewChanged(treeland_dde_shell_surface *handle, bool skip);
    void surfaceSkipMutiTaskViewChanged(treeland_dde_shell_surface *handle, bool skip);

protected:
    void create(WServer *server) override;
    void destroy(WServer *server) override;
    wl_global *global() const override;
    QByteArrayView interfaceName() const override;

private:
    treeland_dde_shell_manager_v1 *m_manager;
    QMap<treeland_window_overlap_checker *, QRect> m_conflictList;
    const int m_shellLayerCount = 1;
};
