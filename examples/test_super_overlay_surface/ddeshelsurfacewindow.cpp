// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "ddeshelsurfacewindow.h"

#include "ddeshellwayland.h"

#include <QLineEdit>
#include <QPushButton>

DDEShelSurfaceWindow::DDEShelSurfaceWindow(TestMode mode, QWidget *parent)
    : QWidget{ parent }
    , m_mode(mode)
{
    QLineEdit *l = new QLineEdit(this);
    
    struct ResizeButton {
        QString text;
        QtWayland::treeland_dde_shell_surface_v1::resize_edge edge;
        QPoint pos;
    };

    const QList<ResizeButton> buttons = {
        { "Right",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_right,
          { 10, 50 } },
        { "Left",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_left,
          { 10, 90 } },
        { "Top",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_top,
          { 10, 130 } },
        { "Bottom",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_bottom,
          { 10, 170 } },
        { "Top Left",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_top_left,
          { 10, 210 } },
        { "Bottom Left",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_bottom_left,
          { 10, 250 } },
        { "Top Right",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_top_right,
          { 10, 290 } },
        { "Bottom Right",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_bottom_right,
          { 10, 330 } },
        { "Cancel",
          QtWayland::treeland_dde_shell_surface_v1::resize_edge::resize_edge_none,
          { 10, 370 } }
    };

    for (const auto &btn : buttons) {
        QPushButton *resizeBtn = new QPushButton(QString("Resize %1").arg(btn.text), this);
        resizeBtn->move(btn.pos);
        connect(resizeBtn, &QPushButton::clicked, this, [this, edge = btn.edge]() {
            if (auto shell = DDEShellWayland::get(windowHandle())) {
                shell->requestResize(edge);
            }
        });
    }
}

void DDEShelSurfaceWindow::showEvent(QShowEvent *event)
{
    if (isVisible()) {
        apply();
    }
}

void DDEShelSurfaceWindow::apply()
{
    if (TestSetPosition == m_mode) {
        // 1 ----Convenient for the client to set the position of the surface
        DDEShellWayland::get(windowHandle())->setPosition(QPoint(100, 100));
        // ----------------------------------------------------------------
    }

    if (TestSetAutoPlace == m_mode) {
        // 2. Set the vertical alignment of the surface within the cursor width,
        // y offset is 30 relative to the cursor bottom.-------------------
        DDEShellWayland::get(windowHandle())->setAutoPlacement(30);

        // Setting this bit will indicate that the window prefers not to be
        // listed in a switcher/dock-preview/mutitask-view
        DDEShellWayland::get(windowHandle())->setSkipDockPreview(true);
        DDEShellWayland::get(windowHandle())->setSkipMutiTaskView(true);
        DDEShellWayland::get(windowHandle())->setSkipSwitcher(true);
        DDEShellWayland::get(windowHandle())->setAcceptKeyboardFocus(false);
        // ---------------------------------------------------------------
    }

    // Do not use setPosition and setAutoPlacement at the same time, there will
    // be conflicts !!!

    DDEShellWayland::get(windowHandle())
        ->setRole(QtWayland::treeland_dde_shell_surface_v1::role_overlay);
}
