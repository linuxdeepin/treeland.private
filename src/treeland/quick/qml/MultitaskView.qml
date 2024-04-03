// Copyright (C) 2024 Yicheng Zhong <zhongyicheng@uniontech.com>.
// SPDX-License-Identif ier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Waylib.Server
import TreeLand
import TreeLand.Utils

Item {
    id: root
    required property var model
    required property int currentWorkspaceId
    required property var setCurrentWorkspaceId
    property int current: -1

    function exit(surfaceItem) {
        if (surfaceItem)
            Helper.activatedSurface = surfaceItem.waylandSurface
        else if (current >= 0 && current < model.count)
            Helper.activatedSurface = model.get(current).item.waylandSurface
        visible = false
    }

    Item {
        id: outputsPlacementItem
        Repeater {
            model: QmlHelper.layout.outputs
            MouseArea {
                id: outputPlacementItem
                function calcDisplayRect(item, output) {
                    // margins on this output
                    const margins = Helper.getOutputExclusiveMargins(output)
                    const coord = parent.mapFromItem(item, 0, 0)
                    // global pos before culling zones
                    const origin = Qt.rect(coord.x, coord.y, item.width, item.height)
                    return Qt.rect(origin.x + margins.left, origin.y + margins.top, 
                        origin.width - margins.left - margins.right, origin.height - margins.top - margins.bottom)
                }
                property rect displayRect: root.visible ? calcDisplayRect(modelData, modelData.output) : Qt.rect(0, 0, 0, 0)
                x: displayRect.x
                y: displayRect.y
                width: displayRect.width
                height: displayRect.height

                onClicked: root.exit()

                ColumnLayout {
                    anchors.fill: parent
                    ListView {
                        id: workspacesList
                        orientation: ListView.Horizontal
                        model: workspaceManager.layoutOrder
                        Layout.preferredHeight: root.height * .2
                        Layout.preferredWidth: contentItem.childrenRect.width
                        Layout.maximumWidth: parent.width
                        Layout.alignment: Qt.AlignHCenter
                        delegate: Item {
                            required property int wsid
                            required property int index
                            height: workspacesList.height
                            width: height * root.width / root.height
                            Rectangle {
                                anchors {
                                    fill: parent
                                    margins: 10
                                }
                                border.color: "blue"
                                border.width: root.currentWorkspaceIndex == index ? 2 : 0
                                ShaderEffectSource {
                                    sourceItem: activeOutputDelegate
                                    anchors.fill: parent
                                }
                                ShaderEffectSource {
                                    sourceItem: workspaceManager.workspacesById.get(wsid)
                                    anchors.fill: parent
                                }
                                HoverHandler {
                                    id: hvrhdlr
                                }
                                TapHandler {
                                    id: taphdlr
                                    onTapped: {
                                        if (root.currentWorkspaceId === index)
                                            multitaskView.active = false
                                        else
                                            root.setCurrentWorkspaceId(index)
                                    }
                                }
                                Rectangle {
                                    anchors.fill: parent
                                    color: Qt.rgba(0,0,0,.2)
                                    visible: hvrhdlr.hovered
                                    Text {
                                        anchors.centerIn: parent
                                        color: "white"
                                        text: `No.${index}`
                                    }
                                }
                            }
                        }
                    }
                    Item {
                        id: surfacesGridView
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        FilterProxyModel {
                            id: outputProxy
                            sourceModel: root.model
                            property bool initialized: false
                            filterAcceptsRow: (d) => {
                                const item = d.item
                                if (!(item instanceof SurfaceItem))
                                    return false
                                return item.waylandSurface.surface.primaryOutput === modelData.output
                            }
                            Component.onCompleted: {
                                initialized = true  // TODO better initialize timing
                                invalidate()
                                grid.calcLayout()
                            }
                            onCountChanged: {
                                grid.calcLayout()
                            }
                            onSourceModelChanged: {
                                invalidate()
                                if (initialized) grid.calcLayout()
                            }
                        }

                        EQHGrid {
                            id: grid
                            anchors.centerIn: parent
                            model: outputProxy
                            minH: 100
                            maxH: parent.height
                            maxW: parent.width
                            availH: parent.height
                            availW: parent.width
                            spacing: 20
                            getRatio: (d) => d.item.width / d.item.height
                            delegate: Item {
                                    property SurfaceItem source: modelData.item
                                    width: modelData.dw
                                    height: width * source.height / source.width
                                    clip: true
                                    property bool highlighted: hvhdlr.hovered
                                    HoverHandler {
                                        id: hvhdlr
                                    }
                                    TapHandler {
                                        onTapped: root.exit(source)
                                    }
                                    Rectangle {
                                        anchors.fill: parent
                                        color: "transparent"
                                        border.width: highlighted ? 2 : 0
                                        border.color: "blue"
                                        radius: 8
                                    }
                                    ShaderEffectSource {
                                        anchors {
                                            fill: parent
                                            margins: 3
                                        }
                                        live: true
                                        // no hidesource, may conflict with workspace thumb
                                        smooth: true
                                        sourceItem: source
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
                                        visible: highlighted
                                        
                                        contentItem: Text {
                                            text: source.waylandSurface.title
                                            elide: Qt.ElideRight
                                        }
                                        background: Rectangle {
                                            color: Qt.rgba(255, 255, 255, .2)
                                            radius: 5
                                        }
                                    }
                                }
                        }
                    }
                }
            }
        }
    }
}
