// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import Waylib.Server
import Treeland.Capture
import Treeland
import org.deepin.dtk as D

CaptureSourceSelector {
    id: captureSourceSelector
    anchors.fill: parent
    component SourceToolButton: Item {
        required property string iconName
        required property int selectionMode
        width: 50
        height: 50
        Rectangle {
            id: bg
            anchors {
                fill: parent
                margins: 5
            }
            color: captureSourceSelector.selectionMode === selectionMode ? Qt.rgba(0,0,0,0.2) : "transparent"
            radius: 8
            D.DciIcon {
                anchors.fill: bg
                name: iconName
                sourceSize {
                    width: 36
                    height: 36
                }
            }
            TapHandler {
                gesturePolicy: TapHandler.WithinBounds
                onTapped: {
                    captureSourceSelector.selectionMode = selectionMode
                }
            }
        }
    }

    Shape {
        id: bgShape
        anchors.fill: parent
        ShapePath {
            strokeWidth: 0
            fillColor: Qt.rgba(0, 0, 0, 0.3)
            startX: 0
            startY: 0
            PathRectangle {
                x: 0
                y: 0
                width: bgShape.width
                height: bgShape.height
            }
            PathMove {
                x: selectionRegion.x
                y: selectionRegion.y
            }
            PathRectangle {
                x: selectionRegion.x
                y: selectionRegion.y
                width: selectionRegion.width
                height: selectionRegion.height
            }
        }
    }

    Control {
        id: captureToolBar
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 10
        visible: toolBarModel.count > 1
        OpacityAnimator on opacity {
            from: 0.0
            to: 1.0
            duration: 300
            easing.type: Easing.OutQuad
        }
        contentItem: Row {
            Repeater {
                model: toolBarModel
                SourceToolButton { }
            }
        }
        background: Item {
            Rectangle {
                id: bgRect
                anchors.fill: parent
                color: Qt.rgba(16, 16, 16, .1)
                radius: 8
                HoverHandler {
                    cursorShape: Qt.ArrowCursor
                    blocking: true
                }
            }
            Blur {
                anchors.fill: bgRect
                radius: 8
            }
            D.BoxShadow {
                anchors.fill: parent
                visible: true
                cornerRadius: 8
                shadowColor: Qt.rgba(0, 0, 0, 0.15)
                shadowOffsetY: 8
                shadowBlur: 20
                hollow: true
            }
            Border {
                anchors.fill: parent
                radius: 8
                insideColor: Qt.rgba(255, 255, 255, 0.05)
            }
        }
    }

    Shape {
        width: selectionRegion.width
        height: selectionRegion.height
        x: selectionRegion.x
        y: selectionRegion.y
        ShapePath {
            strokeWidth: 2
            strokeColor: "white"
            fillColor: "transparent"
            strokeStyle: ShapePath.DashLine
            dashPattern: [ 1, 4 ]
            startX: 0
            startY: 0
            PathLine { x: selectionRegion.width; y: 0 }
            PathLine { x: selectionRegion.width; y: selectionRegion.height }
            PathLine { x: 0; y: selectionRegion.height }
            PathLine { x: 0; y: 0}
        }
    }
}
