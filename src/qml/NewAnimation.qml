// Copyright (C) 2023 justforlxz <justforlxz@gmail.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

ShaderEffectSource {
    id: root

    enum Direction {
        Show = 1,
        Hide
    }

    signal stopped

    visible: false
    clip: true

    required property var target
    required property var direction

    width: target.width
    height: target.height
    live: direction === NewAnimation.Direction.Show
    hideSource: true
    sourceItem: root.target
    transform: [
        Rotation {
            id: rotation
            origin.x: width / 2
            origin.y: height / 2
            axis {
                x: 1
                y: 0
                z: 0
            }
            angle: 0
        }
    ]

    function start() {
        visible = true;
        animation.start();
    }

    function stop() {
        visible = false;
        sourceItem = null;
        stopped();
    }

    Connections {
        target: animation
        function onStopped() {
            stop();
        }
    }

    ParallelAnimation {
        id: animation

        PropertyAnimation {
            target: rotation
            property: "angle"
            duration: 400
            from: root.direction === NewAnimation.Direction.Show ? 75 : 0
            to: root.direction !== NewAnimation.Direction.Show ? 75 : 0
            easing.type: Easing.OutExpo
        }
        PropertyAnimation {
            target: root
            property: "scale"
            duration: 400
            from: root.direction === NewAnimation.Direction.Show ? 0.3 : 1
            to: root.direction !== NewAnimation.Direction.Show ? 0.3 : 1
            easing.type: Easing.OutExpo
        }
        PropertyAnimation {
            target: root
            property: "opacity"
            duration: 400
            from: root.direction === NewAnimation.Direction.Show ? 0 : 1
            to: root.direction !== NewAnimation.Direction.Show ? 0 : 1
            easing.type: Easing.OutExpo
        }
    }
}
