// Copyright (C) 2024 lbwtw <xiaoyaobing@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import org.deepin.dtk 1.0 as D

Component {
    Item {
        property real ratio: item.width / item.height
        property ListView sourceView: ListView.view
        property real contanReftWidth: (sourceView.fullheight - sourceView.titleheight - sourceView.vMargin) * ratio + 2 * (sourceView.vSpacing + sourceView.borderMargin)
        property bool widthLimit: contanReftWidth > sourceView.delegateMaxWidth
        height: sourceView.fullheight
        width: contanReftWidth < sourceView.delegateMinWidth ? sourceView.delegateMinWidth
                                                             : (contanReftWidth > sourceView.delegateMaxWidth
                                                                ? sourceView.delegateMaxWidth : contanReftWidth)

        Item {
            id: contentItem

            anchors.centerIn: parent
            height: sourceView.thumbnailheight
            width: parent.width - 2 * (sourceView.vSpacing + sourceView.borderMargin)

            Rectangle {
                anchors.fill: parent
                opacity: 0.8
            }

            Loader {
                enabled: sourceView.enableDelegateBorders
                anchors.fill: parent
                sourceComponent: sourceView.enableDelegateShadows ? shadow : undefined
            }

            Component {
                id: shadow

                D.BoxShadow {
                    shadowColor: "#000000"
                    opacity: 0.2
                    shadowOffsetY: 2
                    shadowBlur: 6
                    cornerRadius: sourceView.radius
                    hollow: true
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    Layout.preferredHeight: sourceView.titleheight
                    Layout.fillWidth: true
                    RowLayout {
                        spacing: 8
                        anchors {
                            fill: parent
                            leftMargin: sourceView.titleHMargin
                            rightMargin: sourceView.titleHMargin
                        }
                        visible: parent.width > 2 * sourceView.iconSize.width ? true : false

                        Rectangle {
                            Layout.preferredWidth: parent.width > sourceView.iconSize.width ? sourceView.iconSize.width : 0
                            Layout.preferredHeight: parent.height > sourceView.iconSize.height ? sourceView.iconSize.height : 0
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                            color: "yellow"
                        }
                        Text {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
                            text: {
                                const xdg = item.shellSurface
                                const wholeTitle = xdg.appId?.length ? `${xdg.title} - ${xdg.appId}` : xdg.title
                                wholeTitle
                            }
                            elide: Qt.ElideRight
                        }
                    }

                    Rectangle {
                        color: "red"
                        opacity: 0.05
                        anchors {
                            top: parent.bottom
                            left: parent.left
                            right: parent.right
                        }
                        height: sourceView.separatorHeight
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ShaderEffectSource {
                        anchors.centerIn: parent
                        width: parent.width
                        height: widthLimit ? width / ratio : parent.height
                        hideSource: false
                        sourceItem: item
                        recursive: true
                    }
                }
            }

            Rectangle {
                visible: sourceView.enableDelegateBorders
                anchors.fill: parent
                color: "transparent"
                opacity: 0.1
                border {
                    color: "#000000"
                    width: 1
                }
            }
        }

        D.ItemViewport {
            anchors.fill: contentItem
            fixed: true
            sourceItem: contentItem
            radius: sourceView.radius
            hideSource: true
        }
    }
}
