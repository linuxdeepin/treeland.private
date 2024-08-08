// Copyright (C) 2023 justforlxz <justforlxz@gmail.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TreeLand.Greeter
import TreeLand.Utils

FocusScope {
    id: root
    clip: true

    required property var output

    WallpaperController {
        id: wallpaperController
        output: root.output
        lock: true
    }

    Loader {
        active: true
        anchors.fill: parent
        sourceComponent: ShaderEffectSource {
            sourceItem: wallpaperController.proxy
            hideSource: false
            live: true
        }
        opacity: wallpaperController.type === Helper.Normal ? 0 : 1
        Behavior on opacity {
            PropertyAnimation {
                duration: 1000
                easing.type: Easing.OutExpo
            }
        }
    }

    // prevent event passing through greeter
    MouseArea {
        anchors.fill: parent
        enabled: true
    }

    Rectangle {
        id: cover
        anchors.fill: parent
        color: 'black'
        opacity: 0.0
        state: wallpaperController.type === Helper.Normal ? "Normal" : "Scale"
        states: [
            State {
                name: "Normal"
                PropertyChanges {
                    target: cover
                    opacity: 0.0
                }
            },
            State {
                name: "Scale"
                PropertyChanges {
                    target: cover
                    opacity: 0.6
                }
            }
        ]

        transitions: [
            Transition {
                from: "*"
                to: "Normal"
                PropertyAnimation {
                    property: opacity
                    duration: 1000
                    easing.type: Easing.OutExpo
                }
            },
            Transition {
                from: "*"
                to: "Scale"
                PropertyAnimation {
                    property: opacity
                    duration: 1000
                    easing.type: Easing.OutExpo
                }
            }
        ]
    }

    Center {
        id: center
        anchors.fill: parent
        anchors.leftMargin: 50
        anchors.topMargin: 50
        anchors.rightMargin: 50
        anchors.bottomMargin: 50

        focus: true
    }

    Connections {
        target: GreeterModel
        function onStateChanged() {
            switch (GreeterModel.state) {
                case GreeterModel.AuthSucceeded: {
                    center.loginGroup.userAuthSuccessed()
                    center.loginGroup.updateHintMsg(center.loginGroup.normalHint)
                    GreeterModel.quit()
                }
                break
                case GreeterModel.AuthFailed: {
                    center.loginGroup.userAuthFailed()
                    center.loginGroup.updateHintMsg(qsTr("Password is incorrect."))
                }
                break
                case GreeterModel.Quit: {
                    GreeterModel.emitAnimationPlayed()
                    wallpaperController.type = Helper.Normal
                }
                break
            }
        }
    }

    Component.onCompleted: {
        wallpaperController.type = Helper.Scale
    }

    Component.onDestruction: {
        wallpaperController.lock = false
    }
}
