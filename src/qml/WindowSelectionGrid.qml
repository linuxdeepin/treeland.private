// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Treeland
import Waylib.Server
import QtQuick.Effects
import QtQuick.Controls
import org.deepin.dtk as D
import org.deepin.dtk.style as DS

Item {
    id: root
    required property QtObject output
    required property WorkspaceModel workspace
    required property int workspaceListPadding
    required property Item draggedParent
    required property QtObject dragManager
    required property Multitaskview multitaskview

    readonly property real delegateCornerRadius: (ros.rows >= 1 && ros.rows <= 3) ? ros.cornerRadiusList[ros.rows - 1] : ros.cornerRadiusList[2]

    QtObject {
        id: ros // readonly state
        property list<real> cornerRadiusList: [18,12,8] // Should get from system preference
        readonly property int rows: surfaceModel.rows
    }

    WallpaperController {
        id: wallpaperController
        output: outputPlacementItem.output.outputItem.output
        lock: true
        type: WallpaperController.Normal
    }

    ShaderEffectSource {
        sourceItem: wallpaperController.proxy
        recursive: true
        live: true
        smooth: true
        anchors.fill: parent
        hideSource: false
    }

    Blur {
        z: Multitaskview.Background
        anchors.fill: parent
        opacity: multitaskview.taskviewVal
        radiusEnabled: false
    }

    MultitaskviewSurfaceModel {
        id: surfaceModel
        workspace: root.workspace
        layoutArea: root.mapToItem(output.outputItem, Qt.rect(surfaceGridView.x, surfaceGridView.y, surfaceGridView.width, surfaceGridView.height))
    }

    Flickable {
        id: surfaceGridView
        // Do not use anchors here, geometry should be stable as soon as the item is created
        y: workspaceListPadding
        width: parent.width
        height: parent.height - workspaceListPadding
        visible: surfaceModel.modelReady
        Repeater {
            model: surfaceModel
            Item {
                id: surfaceItemDelegate

                required property int index
                required property SurfaceWrapper wrapper
                required property rect geometry
                required property bool padding
                required property int zorder
                property bool needPadding
                property real paddingOpacity
                readonly property rect initialGeometry: surfaceGridView.mapFromItem(output.outputItem, wrapper.geometry)
                property real ratio: wrapper.width / wrapper.height
                property real fullY

                visible: true
                clip: false
                state: drg.active ? "dragging" : multitaskview.state
                x: geometry.x
                y: geometry.y
                z: zorder
                width: geometry.width
                height: geometry.height
                states: [
                    State {
                        name: "initial"
                        PropertyChanges {
                            surfaceItemDelegate {
                                x: initialGeometry.x
                                y: initialGeometry.y
                                width: initialGeometry.width
                                height: initialGeometry.height
                                paddingOpacity: 0
                                needPadding: false
                            }
                        }
                    },
                    State {
                        name: "partial"
                        PropertyChanges {
                            surfaceItemDelegate {
                                x: (geometry.x - initialGeometry.x) * partialGestureFactor + initialGeometry.x
                                y: (geometry.y - initialGeometry.y) * partialGestureFactor + initialGeometry.y
                                width: (geometry.width - initialGeometry.width) * partialGestureFactor + initialGeometry.width
                                height: (geometry.height - initialGeometry.height) * partialGestureFactor + initialGeometry.height
                                paddingOpacity: TreelandConfig.multitaskviewPaddingOpacity * partialGestureFactor
                                needPadding: true
                            }
                        }
                    },
                    State {
                        name: "taskview"
                        PropertyChanges {
                            surfaceItemDelegate {
                                x: geometry.x
                                y: geometry.y
                                width: geometry.width
                                height: geometry.height
                                needPadding: padding
                                paddingOpacity: TreelandConfig.multitaskviewPaddingOpacity
                            }
                        }
                    },
                    State {
                        name: "dragging"
                        PropertyChanges {
                            // restoreEntryValues: true // FIXME: does this restore propery binding?
                            surfaceItemDelegate {
                                parent: draggedParent
                                x: mapToItem(draggedParent, 0, 0).x
                                y: mapToItem(draggedParent, 0, 0).y
                                z: Multitaskview.FloatingItem
                                scale: (Math.max(0, Math.min(drg.activeTranslation.y / fullY, 1)) * (100 - width) + width) / width
                                transformOrigin: Item.Center
                                needPadding: false
                            }
                        }
                    }
                ]

                Behavior on x {enabled:state !== "dragging"; NumberAnimation {duration: TreelandConfig.multitaskviewAnimationDuration; easing.type: TreelandConfig.multitaskviewEasingCurveType} }
                Behavior on y {enabled:state !== "dragging"; NumberAnimation {duration: TreelandConfig.multitaskviewAnimationDuration; easing.type: TreelandConfig.multitaskviewEasingCurveType} }
                Behavior on width {enabled:state !== "dragging"; NumberAnimation {duration: TreelandConfig.multitaskviewAnimationDuration; easing.type: TreelandConfig.multitaskviewEasingCurveType} }
                Behavior on height {enabled:state !== "dragging"; NumberAnimation {duration: TreelandConfig.multitaskviewAnimationDuration; easing.type: TreelandConfig.multitaskviewEasingCurveType} }
                Behavior on paddingOpacity {enabled:state !== "dragging"; NumberAnimation {duration: TreelandConfig.multitaskviewAnimationDuration; easing.type: TreelandConfig.multitaskviewEasingCurveType} }
                Behavior on scale {enabled:state !== "dragging"; NumberAnimation {duration: TreelandConfig.multitaskviewAnimationDuration; easing.type: TreelandConfig.multitaskviewEasingCurveType} }

                D.BoxShadow {
                    id: paddingRectShadow
                    anchors.fill: paddingRect
                    visible: needPadding
                    cornerRadius: delegateCornerRadius
                    shadowColor: Qt.rgba(0, 0, 0, 0.3)
                    shadowOffsetY: 16
                    shadowBlur: 32
                    hollow: true
                }

                Rectangle {
                    id: paddingRect
                    visible: needPadding
                    radius: delegateCornerRadius
                    anchors.fill: parent
                    color: "white"
                    opacity: paddingOpacity
                }

                function conv(y, item = parent) { // convert to draggedParent's coord
                    return mapToItem(draggedParent, mapFromItem(item, 0, y)).y
                }

                property bool highlighted: dragManager.item === null && (hvhdlr.hovered || surfaceCloseBtn.hovered) && surfaceItemDelegate.state === "taskview"
                SurfaceProxy {
                    id: surfaceProxy
                    surface: wrapper
                    live: true
                    fullProxy: true
                    width: parent.width
                    height: width / surfaceItemDelegate.ratio
                    anchors.centerIn: parent
                }
                HoverHandler {
                    id: hvhdlr
                    enabled: !drg.active
                }
                TapHandler {
                    gesturePolicy: TapHandler.WithinBounds
                    onTapped: multitaskview.exit(wrapper)
                }
                DragHandler {
                    id: drg
                    onActiveChanged: {
                        if (active) {
                            dragManager.item = surfaceItemDelegate
                        } else {
                            if (dragManager.accept) {
                                dragManager.accept()
                            }
                            dragManager.item = null
                        }
                    }
                    onGrabChanged: (transition, eventPoint) => {
                                       switch (transition) {
                                           case PointerDevice.GrabExclusive:
                                           fullY = conv(workspaceListPadding, root) - conv(mapToItem(surfaceItemDelegate, eventPoint.position).y, surfaceItemDelegate)
                                           break
                                       }
                                   }
                }
                D.RoundButton {
                    id: surfaceCloseBtn
                    icon.name: "multitaskview_close"
                    icon.width: 26
                    icon.height: 26
                    height: 26
                    width: height
                    visible: surfaceItemDelegate.highlighted
                    anchors {
                        top: parent.top
                        right: parent.right
                        topMargin: -8
                        rightMargin: -8
                    }
                    Item {
                        id: surfaceCloseBtnControl
                        property D.Palette textColor: DS.Style.button.text
                    }
                    textColor: surfaceCloseBtnControl.textColor
                    background: Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                    }
                    onClicked: {
                        surfaceItemDelegate.visible = false
                        wrapper.requestClose()
                    }
                }

                Control {
                    id: titleBox
                    anchors {
                        bottom: parent.bottom
                        horizontalCenter: parent.horizontalCenter
                        margins: 10
                    }
                    width: Math.min(implicitContentWidth + 2 * padding, parent.width * .7)
                    padding: 10
                    visible: highlighted && wrapper.shellSurface.title !== ""

                    contentItem: Text {
                        text: wrapper.shellSurface.title
                        elide: Qt.ElideRight
                    }
                    background: Rectangle {
                        color: Qt.rgba(255, 255, 255, .2)
                        radius: TreelandConfig.titleBoxCornerRadius
                    }
                }
            }
        }
        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: {
                multitaskview.exit()
            }
        }
    }

    Component.onCompleted: {
        surfaceModel.calcLayout()
    }
}
