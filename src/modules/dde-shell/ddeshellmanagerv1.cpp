// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "ddeshellmanagerv1.h"

#include <woutput.h>
#include <wtoplevelsurface.h>
#include <wxdgsurfaceitem.h>

static DDEShellManagerV1 *DDE_SHELL_INSTANCE = nullptr;

Q_LOGGING_CATEGORY(ddeshell, "treeland.protocols.ddeshell", QtDebugMsg);

DDEShellAttached *DDEShellHelper::qmlAttachedProperties(QObject *target)
{
    if (auto *item = qobject_cast<WSurfaceItem *>(target)) {
        return new WindowOverlapChecker(item);
    }

    return nullptr;
}

DDEShellAttached::DDEShellAttached(WSurfaceItem *target, QObject *parent)
    : QObject(parent)
    , m_target(target)
{
}

WindowOverlapChecker::WindowOverlapChecker(WSurfaceItem *target, QObject *parent)
    : DDEShellAttached(target, parent)
{
    WXdgSurfaceItem *xdgSurfaceItem = qobject_cast<WXdgSurfaceItem *>(target);
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(300);

    connect(timer, &QTimer::timeout, this, [this] {
        DDE_SHELL_INSTANCE->checkRegionalConflict(m_target);
    });

    connect(target, &WSurfaceItem::xChanged, [timer] {
        if (timer->isActive()) {
            return;
        }
        timer->start();
    });
    connect(target, &WSurfaceItem::yChanged, [timer] {
        if (timer->isActive()) {
            return;
        }
        timer->start();
    });
    connect(target, &WSurfaceItem::widthChanged, [timer] {
        if (timer->isActive()) {
            return;
        }
        timer->start();
    });
    connect(target, &WSurfaceItem::heightChanged, [timer] {
        if (timer->isActive()) {
            return;
        }
        timer->start();
    });
}

void WindowOverlapChecker::setOverlapped(bool overlapped)
{
    if (m_overlapped == overlapped) {
        return;
    }
    m_overlapped = overlapped;
    Q_EMIT overlappedChanged();
}

DDEShellManagerV1::DDEShellManagerV1(QObject *parent)
    : QObject(parent)
{
    if (DDE_SHELL_INSTANCE) {
        qFatal("There are multiple instances of DDEShellManagerV1");
        return;
    }

    DDE_SHELL_INSTANCE = this;
}

void DDEShellManagerV1::create(WServer *server)
{
    m_manager = treeland_dde_shell_manager_v1::create(server->handle());
    connect(
        m_manager,
        &treeland_dde_shell_manager_v1::windowOverlapCheckerCreated,
        this,
        [this](treeland_window_overlap_checker *handle) {
            connect(handle, &treeland_window_overlap_checker::refresh, this, [this, handle] {
                auto *output = WOutput::fromHandle(qw_output::from(handle->m_output));
                QRegion region(0, 0, output->size().width(), output->size().height());
                QRect checkRect;
                switch (handle->m_anchor) {
                case treeland_window_overlap_checker::Anchor::TOP:
                    checkRect = QRect(0, 0, output->size().width(), handle->m_size.height());
                    break;
                case treeland_window_overlap_checker::Anchor::RIGHT:
                    checkRect = QRect(output->size().width() - handle->m_size.width(),
                                      0,
                                      handle->m_size.width(),
                                      output->size().height());
                    break;
                case treeland_window_overlap_checker::Anchor::BOTTOM:
                    checkRect = QRect(0,
                                      output->size().height() - handle->m_size.height(),
                                      output->size().width(),
                                      handle->m_size.height());
                    break;
                case treeland_window_overlap_checker::Anchor::LEFT:
                    checkRect = QRect(0, 0, output->size().width(), handle->m_size.height());
                    break;
                }
                m_conflictList.insert(handle, checkRect);
            });
            connect(handle, &treeland_window_overlap_checker::before_destroy, this, [this, handle] {
                m_conflictList.remove(handle);
            });
        });
}

void DDEShellManagerV1::checkRegionalConflict(WSurfaceItem *target)
{
    QMapIterator<treeland_window_overlap_checker *, QRect> i(m_conflictList);
    while (i.hasNext()) {
        i.next();
        if (i.value().intersects(
                QRect(target->x(), target->y(), target->width(), target->height()))) {
            i.key()->sendOverlapped(true);
            break;
        } else {
            i.key()->sendOverlapped(false);
        }
    }
}

void DDEShellManagerV1::sendActiveIn(uint32_t reason, WSeat *seat)
{
    if (!m_manager)
        return;

    for (auto handle : m_manager->m_ddeActiveHandles) {
        if (handle->treeland_dde_active_is_mapped_to_wseat(seat)) {
            handle->send_active_in(reason);
        }
    }
}

void DDEShellManagerV1::sendActiveOut(uint32_t reason, WSeat *seat)
{
    if (!m_manager)
        return;

    for (auto handle : m_manager->m_ddeActiveHandles) {
        if (handle->treeland_dde_active_is_mapped_to_wseat(seat)) {
            handle->send_active_out(reason);
        }
    }
}

void DDEShellManagerV1::sendStartDrag(WSeat *seat)
{
    if (!m_manager)
        return;

    for (auto handle : m_manager->m_ddeActiveHandles) {
        if (handle->treeland_dde_active_is_mapped_to_wseat(seat)) {
            handle->send_start_drag();
        }
    }
}

bool DDEShellManagerV1::isDdeShellSurface(WSurface *surface)
{
    if (m_manager) {
        for (auto handle : m_manager->m_surfaceHandles) {
            if (!handle->m_surface_resource) {
                continue;
            }

            if (handle->treeland_dde_shell_surface_is_mapped_to_wsurface(surface)) {
                return true;
            }
        }
    }

    return false;
}

treeland_dde_shell_surface *DDEShellManagerV1::ddeShellSurfaceFromWSurface(WSurface *surface) const
{
    if (m_manager) {
        for (auto handle : m_manager->m_surfaceHandles) {
            if (!handle->m_surface_resource) {
                continue;
            }

            if (handle->treeland_dde_shell_surface_is_mapped_to_wsurface(surface)) {
                return handle;
            }
        }
    }

    return nullptr;
}

void DDEShellManagerV1::destroy(WServer *server)
{
    m_manager->deleteLater();
}

wl_global *DDEShellManagerV1::global() const
{
    return m_manager->global;
}

QByteArrayView DDEShellManagerV1::interfaceName() const
{
    return "treeland_dde_shell_manager_v1";
}
