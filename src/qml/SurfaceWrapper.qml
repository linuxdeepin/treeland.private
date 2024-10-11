// Copyright (C) 2023 JiDe Zhang <zccrs@live.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import Waylib.Server
import TreeLand
import TreeLand.Utils
import TreeLand.Protocols

SurfaceItemFactory {
    id: root

    required property DynamicCreatorComponent creatorCompoment
    required property ToplevelSurface wSurface
    required property string type
    required property int wid

    property var doDestroy: helper.doDestroy
    property var cancelMinimize: helper.cancelMinimize
    property var personalizationMapper: wSurface.Personalization
    property var ddeshellMapper: root.surfaceItem.DDEShell
    property int outputCounter: 0
    property bool isMinimized: helper.isMinimized
    // layer shell should ignore the skip... property
    property bool skipSwitcher: (typeof wSurface === "WaylandLayerSurface") ? ((DDEShellV1.isDdeShellSurface(wSurface.surface)
                                 && DDEShellV1.isSurfaceSkipSwitcherInitialized(wSurface.surface)) ? true : false) : true
    property bool skipDockPreView: (typeof wSurface === "WaylandLayerSurface") ? ((DDEShellV1.isDdeShellSurface(wSurface.surface)
                                    && DDEShellV1.isSurfaceSkipDockPreViewInitialized(wSurface.surface)) ? true : false) : true
    property bool skipMutiTaskView: (typeof wSurface === "WaylandLayerSurface") ? ((DDEShellV1.isDdeShellSurface(wSurface.surface)
                                     && DDEShellV1.isSurfaceSkipMutiTaskViewInitialized(wSurface.surface)) ? true : false) : true
    property alias outputs: outputs
    property alias decoration: decoration
    readonly property XWaylandSurfaceItem asXwayland: {surfaceItem as XWaylandSurfaceItem}
    readonly property XdgSurfaceItem asXdg: {surfaceItem as XdgSurfaceItem}
    readonly property bool hasDecoration: personalizationMapper.noTitlebar || (decoration.enable && !helper.isFullScreen)
    readonly property int fixForLevel: 2
    z: {
        if (Helper.clientName(wSurface.surface) === "dde-desktop") {
            return -100 + 1
        }
        else {
            return 0
        }
    }

    function move(pos) {
        manualMoveResizing = true
        surfaceItem.x = pos.x
        surfaceItem.y = pos.y
        manualMoveResizing = false
    }

    function moveSurfaceToCursor(delegate) {
        // vertical alignment of the surface within the cursor, y-axis offset
        const offset = DDEShellV1.surfaceLayerYOffset(wSurface.surface)
        const x = Helper.cursor.position.x + PersonalizationV1.cursorSize.width / 2
        const y = Helper.cursor.position.y + PersonalizationV1.cursorSize.height + offset
        //preventing displayed beyond the edge of the output. eg: xdg-popup
        move({x, y})
    }

    // put here, otherwise may initialize late
    parent: QmlHelper.workspaceManager.workspacesById.get(wid)

    surfaceItem {
        parent: root.parent
        shellSurface: wSurface
        topPadding: hasDecoration ? decoration.topMargin : 0
        bottomPadding: hasDecoration ? decoration.bottomMargin : 0
        leftPadding: hasDecoration ? decoration.leftMargin : 0
        rightPadding: hasDecoration ? decoration.rightMargin : 0
        focus: wSurface === Helper.activatedSurface
        z: (DDEShellV1.isDdeShellSurface(wSurface.surface)
           && DDEShellV1.isSurfaceLayerInitialized(wSurface.surface))
           ? (Number.MAX_VALUE - fixForLevel - DDEShellV1.shellLayerCount + DDEShellV1.surfaceLayerIndex(wSurface.surface)) : 0
        resizeMode:
            if (!surfaceItem.effectiveVisible)
                SurfaceItem.ManualResize
            else if (stateTransition.running || helper.isMaximize)
                SurfaceItem.SizeToSurface
            else
                SurfaceItem.SizeFromSurface
        onResizeModeChanged: {
            // if surface mapped when not visible, it will change mode to sizefromsurf
            // but mode change is not applied if no resize event happens afterwards, so trigger resize here
            if (surfaceItem.resizeMode != SurfaceItem.ManualResize)
                surfaceItem.resize(surfaceItem.resizeMode)
        }
        onEffectiveVisibleChanged: {
            if (surfaceItem.effectiveVisible) {
                console.assert(surfaceItem.resizeMode !== SurfaceItem.ManualResize,
                               "The surface's resizeMode Shouldn't is ManualResize")
                // Apply the WSurfaceItem's size to wl_surface
                surfaceItem.resize(SurfaceItem.SizeToSurface)
            } else {
                Helper.cancelMoveResize(surfaceItem)
            }
        }

        transitions: Transition {
            id: stateTransition
            PropertyAnimation {
                properties: "x,y,width,height"
                duration: 400
                easing.type: Easing.OutExpo
            }
        }
        states: [
            State {
                name: "default" // avoid applying propertychange on initialize
                PropertyChanges {
                    id: defulatState
                    restoreEntryValues: false
                    target: surfaceItem
                    x: store.normal.x
                    y: store.normal.y
                    width: store.normal.width
                    height: store.normal.height
                }
            },
            State {
                name: "intermediate"
                PropertyChanges {
                    restoreEntryValues: false
                    target: surfaceItem
                    x: store.normal.x
                    y: store.normal.y
                    width: store.normal.width
                    height: store.normal.height
                }
            },
            State {
                name: "maxmize"
                when: helper.isMaximize
                PropertyChanges {
                    restoreEntryValues: true
                    target: surfaceItem
                    x: helper.maximizeRect.x
                    y: helper.maximizeRect.y
                    width: helper.maximizeRect.width
                    height: helper.maximizeRect.height
                }
            },
            State {
                name: "fullscreen"
                when: helper.isFullScreen
                PropertyChanges {
                    restoreEntryValues: true
                    target: surfaceItem
                    x: helper.fullscreenRect.x
                    y: helper.fullscreenRect.y
                    z: 100 + 1 // LayerType.Overlay + 1
                    width: helper.fullscreenRect.width
                    height: helper.fullscreenRect.height
                }
            }
        ]

        delegate: Item {
            required property SurfaceItem surface

            anchors.fill: parent
            SurfaceItemContent {
                id: content
                surface: parent.surface.surface
                anchors.fill: parent
            }

            Component {
                id: radiusCom
                TRadiusEffect {
                    property real contentRadius: Math.min(root.cornerRadius, content.width / 2, content.height / 2)
                    sourceItem: content
                    hideSource: true
                    bottomRightRadius: contentRadius
                    bottomLeftRadius: contentRadius
                }
            }

            Loader {
                id: effectLoader
                anchors.fill: parent
                sourceComponent: GraphicsInfo.api !== GraphicsInfo.Software && root.cornerRadius > 0 ? radiusCom : undefined;
            }
        }
    }

    property var store: ({})
    property int storeNormalWidth: undefined
    property bool isRestoring: false
    property bool aboutToRestore: false
    readonly property real cornerRadius: helper.isMaximize  ? 0 : 15

    property int scaleStatus: 1
    property real scaleTo
    property real xFrom
    property real xTo
    property real yFrom
    property real yTo
    property bool inHomeAni: false

    onScaleStatusChanged: {
        if (scaleStatus === -1 && transitions[0].running && inHomeAni) {
            transitions[0].animations[0].stop()
            surfaceItem.x = xFrom
            surfaceItem.y = yFrom
            surfaceItem.scale = 1.0
            surfaceItem.opacity = 0.0
        }
    }

    transitions: [
        Transition {
            from: "scaleToPreview"
            to: "noScale"
            ParallelAnimation {
                id: parallelAnimation

                readonly property int duration: 500
                ScaleAnimator {
                    target: surfaceItem;
                    from: root.scaleTo;
                    to: 1.0
                    duration: parallelAnimation.duration
                    easing.type: Easing.OutExpo
                }

                OpacityAnimator {
                    target: surfaceItem;
                    from: 0.0
                    to: 1.0
                    duration: parallelAnimation.duration
                    easing.type: Easing.OutExpo
                }

                XAnimator {
                    target: surfaceItem
                    from: root.xTo
                    to: root.xFrom
                    duration: parallelAnimation.duration
                    easing.type: Easing.OutExpo
                }

                YAnimator {
                    target: surfaceItem
                    from: root.yTo
                    to: root.yFrom
                    duration: parallelAnimation.duration
                    easing.type: Easing.OutExpo
                }
            }
        }
    ]
    states: [
        State {
            name: 'noScale'
            when: root.scaleStatus === 0
        },
        State {
            name: 'scaleToPreview'
            when: root.scaleStatus === 1
        }
    ]

    function saveState() {
        let nw = store ?? {}
        console.debug(`store = ${store} state = {x: ${surfaceItem.x}, y: ${surfaceItem.y}, width: ${surfaceItem.width}, height: ${surfaceItem.height}}`)
        nw.normal = {x: surfaceItem.x, y: surfaceItem.y, width: surfaceItem.width, height: surfaceItem.height}
        store = nw
    }
    function restoreState(toStore) {
        console.debug(`restoring state to ${QmlHelper.printStructureObject(toStore.normal)}`)
        surfaceItem.state = "intermediate"
        store = toStore
        surfaceItem.state = "default"
    }
    onIsMoveResizingChanged: if (!isMoveResizing) {
        saveState()
    }
    onStoreChanged: {
        storeNormalWidth = store.normal?.width ?? 0
    }

    ListModel {
        id: outputs
    }

    OutputLayoutItem {
        parent: surfaceItem
        anchors.fill: parent
        layout: Helper.outputLayout

        onEnterOutput: function(output) {
            if (wSurface.surface) {
                wSurface.surface.enterOutput(output)
            }
            Helper.onSurfaceEnterOutput(wSurface, surfaceItem, output)
            outputs.append({'output': output})
            outputCounter++

            if (outputCounter === 1) {
                const pos = QmlHelper.winposManager.nextPos(wSurface.appId, surfaceItem.parent, surfaceItem)
                let outputDelegate = output.OutputItem.item
                // Move to the location specified by the client
                if (DDEShellV1.isDdeShellSurface(wSurface.surface)
                        && DDEShellV1.isSurfacePosInitialized(wSurface.surface)) {
                    move(DDEShellV1.surfacePos(wSurface.surface))
                } else if (DDEShellV1.isDdeShellSurface(wSurface.surface)
                           && DDEShellV1.isSurfaceYOffsetInitialized(wSurface.surface)) {
                    moveSurfaceToCursor(outputDelegate)
                } else {
                    move(pos)
                }

                if (Helper.clientName(wSurface.surface) === "dde-desktop") {
                    surfaceItem.x = outputDelegate.x
                    surfaceItem.y = outputDelegate.y
                    surfaceItem.width = outputDelegate.width
                    surfaceItem.height = outputDelegate.height
                }
            }
        }
        onLeaveOutput: function(output) {
            if (wSurface && wSurface.surface)
                wSurface.surface.leaveOutput(output)
            Helper.onSurfaceLeaveOutput(wSurface, surfaceItem, output)

            for (var i = 0; i < outputs.count; ++i) {
                if (outputs.get(i).output === output) {
                    outputs.remove(i);
                    break;
                }
            }

            outputCounter--

            if (outputCounter == 0 && helper.mapped) {
                const pos = QmlHelper.winposManager.nextPos(wSurface.appId, surfaceItem.parent, surfaceItem)
                move(pos)
            }
        }
    }

    WindowDecoration {
        id: decoration
        parent: surfaceItem
        anchors.fill: parent
        z: SurfaceItem.ZOrder.ContentItem - 1
        surface: wSurface
        visible: hasDecoration
        moveable: !helper.isMaximize && !helper.isFullScreen
        radius: cornerRadius
        noTitlebar: personalizationMapper.noTitlebar
    }

    StackToplevelHelper {
        id: helper
        surface: surfaceItem
        waylandSurface: wSurface
        creator: creatorCompoment
        decoration: decoration
        surfaceWrapper: root
    }

    property bool manualMoveResizing: false
    property bool isMoveResizing: manualMoveResizing || Helper.resizingItem === surfaceItem || Helper.movingItem === surfaceItem

    Connections {
        target: Helper.windowGesture

        onActivated: {
            if (root.wSurface === Helper.activatedSurface) {
                helper.doMaximize()
            }
        }

        onDeactivated: {
            if (root.wSurface === Helper.activatedSurface) {
                helper.cancelMaximize()
            }
        }
    }

    Connections {
        target: DDEShellV1.isDdeShellSurface(wSurface.surface) ? DDEShellV1 : null

        function onSurfacePositionChanged(handle, pos) {
            root.move(pos)
        }

        function onSurfaceLayerChanged(handle, layer) {
            root.surfaceItem.z = (Number.MAX_VALUE - fixForLevel - DDEShellV1.shellLayerCount + DDEShellV1.surfaceLayerIndex(wSurface.surface))
        }

        function onSurfaceYOffsetChanged(handle, offset) {
            moveSurfaceToCursor()
        }

        function onSurfaceSkipSwitcherChanged(handle, skip) {
            root.skipSwitcher = skip
        }

        function onSurfaceSkipDockPreViewChanged(handle, skip) {
            root.skipDockPreView = skip
        }

        function onSurfaceSkipMutiTaskViewChanged(handle, skip) {
            root.skipMutiTaskView = skip
        }
    }
}
