// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "wallpaperimage.h"

#include "helper.h"
#include "wallpapermanager.h"
#include "wallpaperprovider.h"
#include "workspace.h"
#include "woutputitem.h"

#include <woutput.h>

#include <QDir>

WAYLIB_SERVER_USE_NAMESPACE

WallpaperImage::WallpaperImage(QQuickItem *parent)
    : QQuickAnimatedImage(parent)
{
    auto provider = Helper::instance()->qmlEngine()->wallpaperImageProvider();
    connect(provider,
            &WallpaperImageProvider::wallpaperTextureUpdate,
            this,
            &WallpaperImage::updateWallpaperTexture);

    setFillMode(Tile);
    setCache(false);
    setAsynchronous(true);
}

WallpaperImage::~WallpaperImage() { }

int WallpaperImage::userId()
{
    return m_userId;
}

void WallpaperImage::setUserId(const int id)
{
    if (m_userId != id) {
        m_userId = id;
        Q_EMIT userIdChanged();
        updateSource();
    }
}

WorkspaceModel *WallpaperImage::workspace()
{
    return m_workspace;
}

void WallpaperImage::setWorkspace(WorkspaceModel *workspace)
{
    if (m_workspace != workspace) {
        m_workspace = workspace;
        Q_EMIT workspaceChanged();
        updateSource();
    }
}

WOutput *WallpaperImage::output()
{
    return m_output;
}

void WallpaperImage::setOutput(WOutput *output)
{
    if (m_output != output) {
        if (m_output)
            QObject::disconnect(m_output, nullptr, this, nullptr);

        m_output = output;
        Q_EMIT outputChanged();

        if (output) {
            setSourceSize(output->transformedSize());
            connect(output, &WOutput::transformedSizeChanged, this, [this] {
                setSourceSize(m_output->transformedSize());
            });

            WallpaperManager::instance()->add(this, WOutputItem::getOutputItem(output));
        } else {
            WallpaperManager::instance()->remove(this);
        }
        updateSource();
    }
}

void WallpaperImage::updateSource()
{
    if (m_userId == -1 || !m_output || !m_workspace) {
        return;
    }

    auto *provider = Helper::instance()->qmlEngine()->wallpaperImageProvider();
    setSource(provider->source(m_output, m_workspace));
}

void WallpaperImage::updateWallpaperTexture(const QString &id)
{
    // TODO: optimize this
    if (!source().toString().isEmpty()) {
        QFile::remove(source().toString().remove("file://"));
    }
    updateSource();
    load();
    update();
    setPlaying(true);
}
