// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "surfacefilterproxymodel.h"
#include "surfacewrapper.h"

SurfaceFilterProxyModel::SurfaceFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

void SurfaceFilterProxyModel::setFilterAppId(const QString &appid)
{
    m_filterAppId = appid;
    invalidateFilter();
}

bool SurfaceFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    SurfaceWrapper *surface = sourceModel()->data(index).value<SurfaceWrapper*>();

    if (m_filterAppId.isEmpty()) {
        return true;
    }

    if (surface) {
        auto wsurface = surface->shellSurface();
        Q_ASSERT(wsurface);
        return wsurface->appId() == m_filterAppId;
    }

    return false;
}
