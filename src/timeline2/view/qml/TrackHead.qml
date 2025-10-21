/*
    SPDX-FileCopyrightText: 2013-2016 Meltytech LLC
    SPDX-FileCopyrightText: 2013-2016 Dan Dennedy <dan@dennedy.org>

    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: trackHeadRoot
    property string trackName
    property string effectNames
    property bool isStackEnabled
    property bool isDisabled
    property bool collapsed: false
    property int isComposite
    property bool isLocked: false
    property bool isActive: false
    property bool isAudio
    property bool showAudioRecord: false
    property bool current: false
    property int trackId : -42
    property string trackTag
    property int thumbsFormat: 0
    border.width: 1
    border.color: root.frameColor

    function editName() {
        nameEdit.visible = true
        nameEdit.focus = true
        nameEdit.selectAll()
    }

    function animateLock() {
        flashLock.restart();
    }
    
    onShowAudioRecordChanged: {
        if (showAudioRecord && trackHeadRoot.height < 2 * root.collapsedHeight + Math.ceil(root.baseUnit/3)) {
            // Ensure trackheight is large enough to have the vu-meter visible
            timeline.adjustTrackHeight(trackHeadRoot.trackId, 2 * root.collapsedHeight + Math.ceil(root.baseUnit/3))
        }
    }

    color: getTrackColor(isAudio, true)
    //border.color: selected? 'red' : 'transparent'
    //border.width: selected? 1 : 0
    clip: true
    state: 'normal'
    states: [
        State {
            name: 'current'
            when: trackHeadRoot.current
            PropertyChanges {
                target: trackHeadRoot
                color: showAudioRecord ? Qt.tint(getTrackColor(isAudio, true), Qt.rgba(1, 0, 0, 0.16)) : selectedTrackColor
            }
        },
        State {
            when: !trackHeadRoot.current
            name: 'normal'
            PropertyChanges {
                target: trackHeadRoot
                color: showAudioRecord ? Qt.tint(getTrackColor(isAudio, true), Qt.rgba(1, 0, 0, 0.16)) : getTrackColor(isAudio, true)
            }
        }
    ]

    Keys.onDownPressed: {
        root.moveSelectedTrack(1)
    }
    Keys.onUpPressed: {
        root.moveSelectedTrack(-1)
    }

    MouseArea {
        id: headerMouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: mouse => {
            timeline.activeTrack = trackId
            if (mouse.button == Qt.RightButton) {
                root.showHeaderMenu()
            }
        }
        onClicked: mouse => {
            console.log('TRACK ID: ', trackId)
            parent.forceActiveFocus()
            nameEdit.visible = false
            if (mouse.button == Qt.LeftButton) {
                timeline.showTrackAsset(trackId)
            }
        }
    }
    Label {
        id: trackTarget
        property color bgColor: 'grey'
        font: miniFont
        color: timeline.targetTextColor
        background: Rectangle {
            color: trackTarget.bgColor
        }
        width: 1.5 * root.baseUnit
        height: trackHeadRoot.height
        verticalAlignment: Text.AlignTop
        horizontalAlignment: Text.AlignHCenter
        visible: trackHeadRoot.isAudio ? timeline.hasAudioTarget > 0 : timeline.hasVideoTarget
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 1

        MouseArea {
            id: targetArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            onClicked: mouse => {
                if (mouse.button == Qt.RightButton) {
                    if (trackHeadRoot.isAudio) {
                        root.showTargetMenu(trackId)
                    } else {
                        root.showHeaderMenu()
                    }
                }
                else {
                    if (trackHeadRoot.isAudio) {
                        timeline.switchAudioTarget(trackHeadRoot.trackId);
                    } else {
                        if (trackHeadRoot.trackId === timeline.videoTarget) {
                            timeline.videoTarget = -1;
                        } else if (timeline.hasVideoTarget) {
                            timeline.videoTarget = trackHeadRoot.trackId;
                        }
                    }
                }
            }
            ToolButton {
                id: targetMouse
                focusPolicy: Qt.NoFocus
                visible: trackHeadRoot.isAudio && timeline.clipTargets > 1 && trackHeadRoot.height > (2 * expandButton.height)
                background: Rectangle {
                    color: Qt.darker(trackTarget.bgColor, 1.5)
                    border.color: activePalette.light
                }
                anchors.bottom: parent.bottom
                width: parent.width
                height: width
                icon.name: "go-down"
                onClicked: {
                    root.showTargetMenu(trackId)
                }
                ToolTip {
                    visible: targetMouse.hovered
                    font: miniFont
                    text: timeline.actionText("switch_target_stream")
                }
            }
        }
        ToolTip {
            visible: targetArea.containsMouse && !targetMouse.hovered
            font: miniFont
            text: i18n("Click to toggle track as target. Target tracks will receive the inserted clips")
        }
    state:  'normalTarget'
    states: [
        State {
            name: 'target'
            when: (trackHeadRoot.isAudio && timeline.audioTarget.indexOf(trackHeadRoot.trackId) > -1) || (!trackHeadRoot.isAudio && trackHeadRoot.trackId === timeline.videoTarget)
            PropertyChanges {
                target: trackTarget
                bgColor: timeline.targetColor
                text: trackHeadRoot.isAudio ? timeline.audioTargetName(trackHeadRoot.trackId) : ''
            }
        },
        State {
            name: 'inactiveTarget'
            when: (trackHeadRoot.isAudio && timeline.lastAudioTarget.indexOf(trackHeadRoot.trackId) > -1) || (!trackHeadRoot.isAudio && trackHeadRoot.trackId == timeline.lastVideoTarget)
            PropertyChanges {
                target: trackTarget
                opacity: 0.3
                bgColor: activePalette.text
                text: trackHeadRoot.isAudio ? timeline.audioTargetName(trackHeadRoot.trackId) : ''
            }
        },
        State {
            name: 'noTarget'
            when: !trackHeadRoot.isLocked && !trackHeadRoot.isDisabled
            PropertyChanges {
                target: trackTarget
                bgColor: activePalette.base
                text: ''
            }
        }
    ]
    transitions: [
        Transition {
            to: '*'
            ColorAnimation { target: trackTarget; duration: 300 }
        }
    ]
    }
    Item {
        id: trackHeadColumn
        anchors.fill: parent
        anchors.leftMargin: trackTarget.width
        anchors.topMargin: 0

        ToolButton {
            id: expandButton
            focusPolicy: Qt.NoFocus
            property int modifier: 0
            icon.name: trackHeadRoot.collapsed ? "go-next" : "go-down"
            onClicked: {
                if (modifier & Qt.ShiftModifier) {
                    // Collapse / expand all tracks
                    timeline.collapseAllTrackHeight(trackId, !trackHeadRoot.collapsed, root.collapsedHeight)
                } else {
                    if (trackHeadRoot.collapsed) {
                        var newHeight = Math.max(root.collapsedHeight * 1.5, controller.getTrackProperty(trackId, "kdenlive:trackheight"))
                        controller.setTrackProperty(trackId, "kdenlive:trackheight", newHeight)
                        controller.setTrackProperty(trackId, "kdenlive:collapsed", "0")
                    } else {
                        controller.setTrackProperty(trackId, "kdenlive:collapsed", root.collapsedHeight)
                    }
                }
                if (root.autoTrackHeight) {
                    timeline.autofitTrackHeight(scrollView.height - subtitleTrack.height, root.collapsedHeight)
                }
            }
            MouseArea {
                // Used to pass modifier state to expand button
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                onPressed: mouse => {
                    expandButton.modifier = mouse.modifiers
                    mouse.accepted = false
                }
                onEntered: {
                    timeline.showKeyBinding(i18n("<b>Shift</b> to collapse/expand all tracks of the same type (audio/video)"))
                }
                onExited: {
                    timeline.showKeyBinding()
                }
            }
            anchors.left: parent.left
            width: root.collapsedHeight
            height: root.collapsedHeight
            ToolTip {
                visible: expandButton.hovered
                font: miniFont
                text: trackLabel.visible? i18n("Minimize") : i18n("Expand")
            }
        }
        Label {
            id: trackLed
            property color bgColor: Qt.darker(trackHeadRoot.color, 0.55)
            anchors.left: expandButton.right
            font: miniFont
            text: trackHeadRoot.trackTag
            color: activePalette.text
            background: Rectangle {
                color: trackLed.bgColor
            }
            width: root.trackTagWidth
            height: root.collapsedHeight - 2
            y: 1
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                id: tagMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    timeline.switchTrackActive(trackHeadRoot.trackId)
                }
            }
            ToolTip {
                visible: tagMouseArea.containsMouse
                font: miniFont
                text: (trackHeadRoot.trackName.length == 0 || miniTrackLabel.visible || trackLabel.visible) ? i18n("Click to make track active/inactive. Active tracks will react to editing operations") : trackHeadRoot.trackName
            }
        state:  'normalled'
            states: [
                State {
                    name: 'locked'
                    when: trackHeadRoot.isLocked
                    PropertyChanges {
                        target: trackLed
                        bgColor: 'red'
                    }
                },
                State {
                    name: 'active'
                    when: trackHeadRoot.isActive
                    PropertyChanges {
                        target: trackLed
                        bgColor: timeline.targetColor
                        color: timeline.targetTextColor
                    }
                },
                State {
                    name: 'inactive'
                    when: !trackHeadRoot.isLocked && !trackHeadRoot.isActive
                    PropertyChanges {
                        target: trackLed
                        bgColor: Qt.darker(trackHeadRoot.color, 0.55)
                    }
                }
            ]
            transitions: [
                Transition {
                    to: '*'
                    ColorAnimation { target: trackLed; duration: 300 }
                }
            ]
        }
        Label {
            // Debug: trackId
            anchors.left: trackLed.right
            background: Rectangle {
                color: 'magenta'
            }
            anchors.leftMargin: 2
            width: root.trackTagWidth * 2
            height: root.collapsedHeight - 2
            y: 1
            text: trackHeadRoot.trackId
            elide: Text.ElideRight
            font: miniFont
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            visible: root.debugmode
        }
        Label {
            id: miniTrackLabel
            anchors.left: trackLed.right
            anchors.top: parent.top
            anchors.leftMargin: 2
            height: trackLed.height
            width: buttonsRow.x - x
            text: trackHeadRoot.trackName
            elide: Text.ElideRight
            font: miniFont
            color: activePalette.text
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            visible: !trackLabel.visible && trackHeadRoot.width > (trackTarget.width + expandButton.width + trackLed.width + (4 * muteButton.width) + 4)
        }
        Row {
            id: buttonsRow
            width: childrenRect.width
            x: Math.max(2 * root.collapsedHeight + 2, parent.width - width - 4)
            spacing: 0
            ToolButton {
                id: effectButton
                focusPolicy: Qt.NoFocus
                icon.name: "tools-wizard"
                enabled: trackHeadRoot.effectNames != ''
                checkable: true
                checked: enabled && trackHeadRoot.isStackEnabled
                onClicked: {
                    timeline.showTrackAsset(trackId)
                    controller.setTrackStackEnabled(trackId, !isStackEnabled)
                }
                width: root.collapsedHeight
                height: root.collapsedHeight
                ToolTip {
                    visible: effectButton.hovered
                    font: miniFont
                    text: enabled ? (isStackEnabled ? i18n("Disable track effects") : i18n("Enable track effects")) : i18n("Toggle track effects");
                }
            }
            ToolButton {
                id: muteButton
                property int modifier: 0
                focusPolicy: Qt.NoFocus
                icon.name: isAudio ? (isDisabled ? "audio-off" : "audio-volume-high") : (isDisabled ? "kdenlive-hide-video" : "kdenlive-show-video")
                width: root.collapsedHeight
                height: root.collapsedHeight
                onClicked: timeline.hideTrack(trackId, isDisabled, modifier & Qt.ShiftModifier)
                MouseArea {
                    // Used to pass modifier state to expand button
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    onPressed: mouse => {
                        muteButton.modifier = mouse.modifiers
                        mouse.accepted = false
                    }
                    onEntered: {
                        timeline.showKeyBinding(i18n("<b>Shift</b> to show/hide all tracks of the same type (audio/video)"))
                    }
                    onExited: {
                        timeline.showKeyBinding()
                    }
                }
                ToolTip {
                    visible: muteButton.hovered
                    font: miniFont
                    text: isAudio ? (isDisabled? i18n("Unmute") : i18n("Mute")) : (isDisabled? i18n("Show") : i18n("Hide"))
                }
            }

            ToolButton {
                id: lockButton
                width: root.collapsedHeight
                height: root.collapsedHeight
                focusPolicy: Qt.NoFocus
                icon.name: trackHeadRoot.isLocked ? "lock" : "unlock"
                onClicked: controller.setTrackLockedState(trackId, !isLocked)
                ToolTip {
                    visible: lockButton.hovered
                    font: miniFont
                    text: isLocked? i18n("Unlock track") : i18n("Lock track")
                }
                SequentialAnimation {
                    id: flashLock
                    loops: 3
                    ParallelAnimation {
                        ScaleAnimator {target: lockButton; from: 1; to: 1.2; duration: 120}
                    }
                    ParallelAnimation {
                        ScaleAnimator {target: lockButton; from: 1.6; to: 1; duration: 120}
                    }
                }
            }
        }
        Item {
            id: recLayout
            y: root.collapsedHeight + 4
            anchors.left: trackHeadColumn.left
            anchors.right: trackHeadColumn.right
            anchors.margins: 2
            height: showAudioRecord ? root.collapsedHeight - 4 : 0
            Loader {
                id: audioVuMeter
                asynchronous: true 
                anchors.fill: parent
                visible: showAudioRecord && (trackHeadRoot.height >= 2 * root.collapsedHeight + Math.ceil(root.baseUnit/3))
                source: isAudio && showAudioRecord ? "AudioRecordingControls.qml" : ""
                onLoaded: item.trackId = trackId
            }
        }
        Item {
            id: nameEditContainer
            anchors.bottom: trackHeadColumn.bottom
            anchors.left: trackHeadColumn.left
            anchors.right: trackHeadColumn.right
            anchors.margins: 2
            anchors.rightMargin: 2
            height: nameEdit.height
            Rectangle {
                id: trackLabel
                color: 'transparent'
                radius: 2
                anchors.fill: parent
                border.color: trackNameMouseArea.containsMouse ? activePalette.highlight : 'transparent'
                visible: (trackHeadRoot.height >= trackLabel.height + muteButton.height + resizer.height + recLayout.height)
                MouseArea {
                    id: trackNameMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true
                    cursorShape: Qt.IBeamCursor
                    onDoubleClicked: editName()
                    onClicked: {
                        timeline.showTrackAsset(trackId)
                        timeline.activeTrack = trackId
                        trackHeadRoot.focus = true
                    }
                }
                Label {
                    text: trackName
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    elide: Qt.ElideRight
                    font: miniFont
                    color: activePalette.text
                }
                Label {
                    id: placeHolder
                    visible: trackName == '' && (trackNameMouseArea.containsMouse || headerMouseArea.containsMouse)
                    enabled: false
                    text: i18n("Edit track name")
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    elide: Qt.ElideRight
                    font: miniFont
                    color: activePalette.text
                }
                TextField {
                    id: nameEdit
                    visible: false
                    width: parent.width
                    text: trackName
                    font: miniFont
                    color: activePalette.text
                    background: Rectangle {
                        radius: 2
                        color: activePalette.window
                        anchors.fill: parent
                    }
                    /*style: TextFieldStyle {
                        padding.top:0
                        padding.bottom: 0
                        background: Rectangle {
                            radius: 2
                            color: activePalette.window
                            anchors.fill: parent
                        }
                    }*/
                    onEditingFinished: {
                        controller.setTrackName(trackId, text)
                        tracksArea.focus = true
                        visible = false
                    }
                }
            }
        }
    }
    Rectangle {
            id: resizer
            height: Math.round(root.baseUnit/3)
            color: 'red'
            opacity: 0
            Drag.active: trimInMouseArea.drag.active
            Drag.proposedAction: Qt.MoveAction
            width: trackHeadRoot.width
            y: trackHeadRoot.height - height

            MouseArea {
                id: trimInMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeVerCursor
                drag.target: parent
                drag.axis: Drag.YAxis
                drag.minimumY: root.collapsedHeight - resizer.height
                property double startY
                property double originalY
                drag.smoothed: false
                property bool dragStarted: false

                onPressed: {
                    root.autoScrolling = false
                    dragStarted = false
                    startY = mapToItem(null, x, y).y
                    originalY = trackHeadRoot.height // reusing originalX to accumulate delta for bubble help
                }
                onReleased: mouse => {
                    root.autoScrolling = timeline.autoScroll
                    if (!trimInMouseArea.containsMouse) {
                        parent.opacity = 0
                    }
                    if (mouse.modifiers & Qt.ShiftModifier && dragStarted) {
                        timeline.adjustAllTrackHeight(trackHeadRoot.trackId, trackHeadRoot.height)
                    }
                }
                onEntered: parent.opacity = 0.3
                onExited: parent.opacity = 0
                onDoubleClicked: mouse => {
                    timeline.defaultTrackHeight(mouse.modifiers & Qt.ShiftModifier ? -1 : trackHeadRoot.trackId)
                }
                onPositionChanged: mouse=> {
                    if (mouse.buttons === Qt.LeftButton) {
                        parent.opacity = 0.5
                        if (!dragStarted && Math.abs(mapToItem(null, x, y).y - startY) > 2) {
                            dragStarted = true
                        }
                        var newHeight = Math.round(originalY + (mapToItem(null, x, y).y - startY))
                        newHeight =  Math.max(root.collapsedHeight, newHeight)
                        if (newHeight == root.collapsedHeight) {
                            controller.setTrackProperty(trackHeadRoot.trackId, "kdenlive:collapsed", root.collapsedHeight)
                        } else {
                            controller.setTrackProperty(trackHeadRoot.trackId, "kdenlive:trackheight", newHeight)
                            controller.setTrackProperty(trackHeadRoot.trackId, "kdenlive:collapsed", "0")
                        }
                    }
                }
            }
        }
    DropArea { //Drop area for tracks
        anchors.fill: trackHeadRoot
        anchors.bottomMargin: resizer.height
        keys: 'kdenlive/effect'
        property string dropData
        property string dropSource
        onEntered: drag => {
            dropData = drag.getDataAsString('kdenlive/effect')
            dropSource = drag.getDataAsString('kdenlive/effectsource')
            drag.acceptProposedAction()
        }
        onDropped: drag => {
            console.log("Add effect: ", dropData)
            if (dropSource == '') {
                // drop from effects list
                controller.addTrackEffect(trackHeadRoot.trackId, dropData);
            } else {
                controller.copyTrackEffect(trackHeadRoot.trackId, dropSource);
            }
            drag.acceptProposedAction()
        }
    }
}

