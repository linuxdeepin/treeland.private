// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "ddeshelsurfacewindow.h"
#include "ddeshellwayland.h"

DDEShelSurfaceWindow::DDEShelSurfaceWindow(QWidget *parent)
    : QWidget{ parent }
{

}

void DDEShelSurfaceWindow::showEvent(QShowEvent *event)
{
    if (isVisible()) {
        apply();
    }
}

void DDEShelSurfaceWindow::apply()
{
    // Convenient for the client to set the position of the surface
    DDEShellWayland::get(windowHandle())->setPosition(QPoint(100, 100));

    // Set the vertical alignment of the surface within the cursor width,
    // y offset is 30 relative to the cursor bottom
    // DDEShellWayland::get(windowHandle())->setAutoPlacement(30);

    DDEShellWayland::get(windowHandle())->setLayer(QtWayland::treeland_dde_shell_surface_v1::layer_super_overlay);
}
