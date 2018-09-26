/*
 * Copyright (c) 2013-2016 Meltytech, LLC
 * Author: Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.6
import QtQuick.Controls 2.2
import Kdenlive.Controls 1.0
import QtQml.Models 2.2
import QtQuick.Window 2.2
import 'Timeline.js' as Logic
import com.enums 1.0

Rectangle {
    id: clipRoot
    property real timeScale: 1.0
    property string clipName: ''
    property string clipResource: ''
    property string mltService: ''
    property string effectNames
    property int modelStart: x
    // Used to store the current frame on move
    property int currentFrame: -1
    property real scrollX: 0
    property int inPoint: 0
    property int outPoint: 0
    property int clipDuration: 0
    property bool isAudio: false
    property bool isComposition: false
    property bool showKeyframes: false
    property bool isGrabbed: false
    property bool grouped: false
    property var audioLevels
    property var markers
    property var keyframeModel
    property var clipStatus: 0
    property var clipType: 0
    property int fadeIn: 0
    property int fadeOut: 0
    property int binId: 0
    property var parentTrack
    property int trackIndex //Index in track repeater
    property int trackId   //Id in the model
    property int clipId     //Id of the clip in the model
    property int originalTrackId: -1
    property int originalX: x
    property int originalDuration: clipDuration
    property int lastValidDuration: clipDuration
    property int draggedX: x
    property bool selected: false
    property bool hasAudio
    property bool canBeAudio
    property bool canBeVideo
    property string hash: 'ccc' //TODO
    property double speed: 1.0
    property color borderColor: 'black'
    property bool forceReloadThumb: false
    width : clipDuration * timeScale;

    signal trimmingIn(var clip, real newDuration, var mouse, bool shiftTrim)
    signal trimmedIn(var clip, bool shiftTrim)
    signal trimmingOut(var clip, real newDuration, var mouse, bool shiftTrim)
    signal trimmedOut(var clip, bool shiftTrim)

    SequentialAnimation on color {
        id: flashclip
        loops: 2
        running: false
        ColorAnimation { from: Qt.darker(getColor()); to: "#ff3300"; duration: 100 }
        ColorAnimation { from: "#ff3300"; to: Qt.darker(getColor()); duration: 100 }
    }

    onIsGrabbedChanged: {
        if (clipRoot.isGrabbed) {
            clipRoot.forceActiveFocus();
            mouseArea.focus = true
        }
    }

    onInPointChanged: {
        if (parentTrack.isAudio) {
            thumbsLoader.item.reload()
        }
    }

    onClipResourceChanged: {
        if (clipType == ProducerType.Color) {
            color: Qt.darker(getColor())
        }
    }

    ToolTip {
        visible: mouseArea.containsMouse && !dragProxyArea.pressed
        font.pixelSize: root.baseUnit
        delay: 1000
        timeout: 5000
        background: Rectangle {
            color: activePalette.alternateBase
            border.color: activePalette.light
        }
        contentItem: Label {
            color: activePalette.text
            text: clipRoot.clipName + ' (' + timeline.timecode(clipRoot.inPoint) + '-' + timeline.timecode(clipRoot.outPoint) + ')'
        }
    }

    onKeyframeModelChanged: {
        console.log('keyframe model changed............')
        if (effectRow.keyframecanvas) {
            effectRow.keyframecanvas.requestPaint()
        }
    }

    onClipDurationChanged: {
        width = clipDuration * timeScale;
    }

    onModelStartChanged: {
        x = modelStart * timeScale;
    }

    onTrackIdChanged: {
        console.log('WARNING CLIP TRACK ID CHANGED: ', trackId)
        clipRoot.parentTrack = Logic.getTrackById(trackId)
        clipRoot.y = clipRoot.originalTrackId == -1 || trackId == originalTrackId ? 0 : parentTrack.y - Logic.getTrackById(clipRoot.originalTrackId).y;
    }

    onForceReloadThumbChanged: {
        // TODO: find a way to force reload of clip thumbs
        thumbsLoader.item.reload()
    }

    onTimeScaleChanged: {
        x = modelStart * timeScale;
        width = clipDuration * timeScale;
        labelRect.x = scrollX > modelStart * timeScale ? scrollX - modelStart * timeScale : 0
        if (parentTrack && parentTrack.isAudio) {
            thumbsLoader.item.reload();
        }
    }
    onScrollXChanged: {
        labelRect.x = scrollX > modelStart * timeScale ? scrollX - modelStart * timeScale : 0
    }

    SystemPalette { id: activePalette }
    color: Qt.darker(getColor())

    border.color: selected? 'red' : grouped ? 'yellowgreen' : borderColor
    border.width: isGrabbed ? 8 : 1.5
    Drag.active: mouseArea.drag.active
    Drag.proposedAction: Qt.MoveAction
    opacity: Drag.active? 0.5 : 1.0

    function getColor() {
        if (clipStatus == ClipState.Disabled) {
            return 'grey'
        }
        if (clipType == ProducerType.Color) {
            var color = clipResource.substring(clipResource.length - 9)
            if (color[0] == '#') {
                return color
            }
            return '#' + color.substring(color.length - 8, color.length - 2)
        }
        return isAudio? '#445f5a' : '#416e8c'
    }

/*    function reparent(track) {
        console.log('TrackId: ',trackId)
        parent = track
        height = track.height
        parentTrack = track
        trackId = parentTrack.trackId
        console.log('Reparenting clip to Track: ', trackId)
        //generateWaveform()
    }
*/
    property bool variableThumbs: (isAudio || clipType == ProducerType.Color || mltService === '')
    property bool isImage: clipType == ProducerType.Image
    property string baseThumbPath: variableThumbs ? '' : 'image://thumbnail/' + binId + '/' + (isImage ? '#0' : '#')
    property string inThumbPath: (variableThumbs || isImage ) ? baseThumbPath : baseThumbPath + Math.floor(inPoint * speed)
    property string outThumbPath: (variableThumbs || isImage ) ? baseThumbPath : baseThumbPath + Math.floor(outPoint * speed)

    DropArea { //Drop area for clips
        anchors.fill: clipRoot
        keys: 'kdenlive/effect'
        property string dropData
        property string dropSource
        property int dropRow: -1
        onEntered: {
            dropData = drag.getDataAsString('kdenlive/effect')
            dropSource = drag.getDataAsString('kdenlive/effectsource')
        }
        onDropped: {
            console.log("Add effect: ", dropData)
            if (dropSource == '') {
                // drop from effects list
                controller.addClipEffect(clipRoot.clipId, dropData);
            } else {
                controller.copyClipEffect(clipRoot.clipId, dropSource);
            }
            dropSource = ''
            dropRow = -1
            drag.acceptProposedAction
        }
    }

    onAudioLevelsChanged: {
        if (parentTrack && parentTrack.isAudio) {
            thumbsLoader.item.reload()
        }
    }
    MouseArea {
        id: mouseArea
        visible: root.activeTool === 0
        anchors.fill: clipRoot
        acceptedButtons: Qt.RightButton
        hoverEnabled: true
        cursorShape: dragProxyArea.drag.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        onPressed: {
            root.stopScrolling = true
            if (mouse.button == Qt.RightButton) {
                clipMenu.item.clipId = clipRoot.clipId
                clipMenu.item.clipStatus = clipRoot.clipStatus
                clipMenu.item.grouped = clipRoot.grouped
                clipMenu.item.trackId = clipRoot.trackId
                clipMenu.item.canBeAudio = clipRoot.canBeAudio
                clipMenu.item.canBeVideo = clipRoot.canBeVideo
                clipMenu.item.popup()
            }
        }
        Keys.onShortcutOverride: event.accepted = clipRoot.isGrabbed && (event.key === Qt.Key_Left || event.key === Qt.Key_Right || event.key === Qt.Key_Up || event.key === Qt.Key_Down)
        Keys.onLeftPressed: {
            controller.requestClipMove(clipRoot.clipId, clipRoot.trackId, clipRoot.modelStart - 1, true, true, true);
        }
        Keys.onRightPressed: {
            controller.requestClipMove(clipRoot.clipId, clipRoot.trackId, clipRoot.modelStart + 1, true, true, true);
        }
        Keys.onUpPressed: {
            controller.requestClipMove(clipRoot.clipId, controller.getNextTrackId(clipRoot.trackId), clipRoot.modelStart, true, true, true);
        }
        Keys.onDownPressed: {
            controller.requestClipMove(clipRoot.clipId, controller.getPreviousTrackId(clipRoot.trackId), clipRoot.modelStart, true, true, true);
        }
        onPositionChanged: {
            var mapped = parentTrack.mapFromItem(clipRoot, mouse.x, mouse.y).x
            root.mousePosChanged(Math.round(mapped / timeline.scaleFactor))
        }
        onEntered: {
            var itemPos = mapToItem(tracksContainerArea, 0, 0, width, height)
            initDrag(clipRoot, itemPos, clipRoot.clipId, clipRoot.modelStart, clipRoot.trackId, false)
        }
        onExited: {
            endDrag()
        }
        onDoubleClicked: {
            drag.target = undefined
            if (mouse.modifiers & Qt.ShiftModifier) {
                if (keyframeModel && showKeyframes) {
                    // Add new keyframe
                    var xPos = Math.round(mouse.x  / timeline.scaleFactor)
                    var yPos = (clipRoot.height - mouse.y) / clipRoot.height
                    keyframeModel.addKeyframe(xPos + clipRoot.inPoint, yPos)
                } else {
                    timeline.position = clipRoot.x / timeline.scaleFactor
                }
            } else {
                timeline.editItemDuration(clipId)
            }
        }
        onWheel: zoomByWheel(wheel)
    }

    Item {
        // Clipping container
        id: container
        anchors.fill: parent
        anchors.margins:1.5
        clip: true
        Loader {
            id: thumbsLoader
            anchors.fill: parent
            source: parentTrack.isAudio ? "ClipAudioThumbs.qml" : clipType == ProducerType.Color ? "" : "ClipThumbs.qml"
        }

        Rectangle {
            // text background
            id: labelRect
            color: clipRoot.selected ? 'darkred' : '#66000000'
            width: label.width + 2
            height: label.height
            visible: clipRoot.width > width / 2
            Text {
                id: label
                text: clipName + (clipRoot.speed != 1.0 ? ' [' + Math.round(clipRoot.speed*100) + '%]': '')
                font.pixelSize: root.baseUnit * 1.2
                anchors {
                    top: labelRect.top
                    left: labelRect.left
                    topMargin: 1
                    leftMargin: 1
                }
                color: 'white'
                style: Text.Outline
                styleColor: 'black'
            }
        }
        Rectangle {
            // effects
            id: effectsRect
            color: '#555555'
            width: effectLabel.width + 2
            height: effectLabel.height
            x: labelRect.x
            anchors.top: labelRect.bottom
            visible: labelRect.visible && clipRoot.effectNames != ''
            Text {
                id: effectLabel
                text: clipRoot.effectNames
                font.pixelSize: root.baseUnit * 1.2
                anchors {
                    top: effectsRect.top
                    left: effectsRect.left
                    topMargin: 1
                    leftMargin: 1
                    // + ((isAudio || !settings.timelineShowThumbnails) ? 0 : inThumbnail.width) + 1
                }
                color: 'white'
                //style: Text.Outline
                styleColor: 'black'
            }
        }

        Repeater {
            model: markers
            delegate:
            Item {
                anchors.fill: parent
                Rectangle {
                    id: markerBase
                    width: 1
                    height: parent.height
                    x: (model.frame - clipRoot.inPoint) * timeScale;
                    color: model.color
                }
                Rectangle {
                    visible: mlabel.visible
                    opacity: 0.7
                    x: markerBase.x
                    radius: 2
                    width: mlabel.width + 4
                    height: mlabel.height
                    anchors {
                        bottom: parent.verticalCenter
                    }
                    color: model.color
                    MouseArea {
                        z: 10
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onDoubleClicked: timeline.editMarker(clipRoot.binId, model.frame)
                        onClicked: timeline.position = (clipRoot.x + markerBase.x) / timeline.scaleFactor
                    }
                }
                Text {
                    id: mlabel
                    visible: timeline.showMarkers && parent.width > width * 1.5
                    text: model.comment
                    font.pixelSize: root.baseUnit
                    x: markerBase.x
                    anchors {
                        bottom: parent.verticalCenter
                        topMargin: 2
                        leftMargin: 2
                    }
                    color: 'white'
                }
            }
        }

        KeyframeView {
            id: effectRow
            visible: clipRoot.showKeyframes && clipRoot.keyframeModel
            selected: clipRoot.selected
            inPoint: clipRoot.inPoint
            outPoint: clipRoot.outPoint
        }
    }

    states: [
        State {
            name: 'normal'
            when: clipRoot.selected === false
            PropertyChanges {
                target: clipRoot
                color: Qt.darker(getColor())
            }
        },
        State {
            name: 'selected'
            when: clipRoot.selected === true
            PropertyChanges {
                target: clipRoot
                z: 1
                color: getColor()
            }
        }
    ]


    TimelineTriangle {
        id: fadeInTriangle
        fillColor: 'green'
        width: Math.min(clipRoot.fadeIn * timeScale, clipRoot.width)
        height: clipRoot.height - clipRoot.border.width * 2
        anchors.left: clipRoot.left
        anchors.top: clipRoot.top
        anchors.margins: clipRoot.border.width
        opacity: 0.3
    }
    Rectangle {
        id: fadeInControl
        anchors.left: fadeInTriangle.width > radius? undefined : fadeInTriangle.left
        anchors.horizontalCenter: fadeInTriangle.width > radius? fadeInTriangle.right : undefined
        anchors.top: fadeInTriangle.top
        anchors.topMargin: -10
        width: root.baseUnit * 2
        height: width
        radius: width / 2
        color: '#FF66FFFF'
        border.width: 2
        border.color: 'green'
        opacity: 0
        Drag.active: fadeInMouseArea.drag.active
        MouseArea {
            id: fadeInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            drag.target: parent
            drag.minimumX: -root.baseUnit * 2
            drag.maximumX: container.width
            drag.axis: Drag.XAxis
            property int startX
            property int startFadeIn
            onEntered: parent.opacity = 0.7
            onExited: {
                if (!pressed) {
                  parent.opacity = 0
                }
            }
            drag.smoothed: false
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeIn = clipRoot.fadeIn
                parent.anchors.left = undefined
                parent.anchors.horizontalCenter = undefined
                parent.opacity = 1
                fadeInTriangle.opacity = 0.5
                // parentTrack.clipSelected(clipRoot, parentTrack) TODO
            }
            onReleased: {
                root.stopScrolling = false
                fadeInTriangle.opacity = 0.3
                parent.opacity = 0
                if (fadeInTriangle.width > parent.radius)
                    parent.anchors.horizontalCenter = fadeInTriangle.right
                else
                    parent.anchors.left = fadeInTriangle.left
                console.log('released fade: ', clipRoot.fadeIn)
                timeline.adjustFade(clipRoot.clipId, 'fadein', clipRoot.fadeIn, startFadeIn)
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((parent.x - startX) / timeScale)
                    if (delta != 0) {
                        var duration = Math.max(0, startFadeIn + delta)
                        duration = Math.min(duration, clipRoot.clipDuration)
                        if (clipRoot.fadeIn - 1 != duration) {
                            timeline.adjustFade(clipRoot.clipId, 'fadein', duration, -1)
                        }
                        // Show fade duration as time in a "bubble" help.
                        var s = timeline.timecode(Math.max(duration, 0))
                        bubbleHelp.show(clipRoot.x, parentTrack.y + clipRoot.height, s)
                    }
                }
            }
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeInMouseArea.containsMouse && !fadeInMouseArea.pressed
            NumberAnimation {
                from: 1.0
                to: 0.7
                duration: 250
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: 0.7
                to: 1.0
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    TimelineTriangle {
        id: fadeOutCanvas
        fillColor: 'red'
        width: Math.min(clipRoot.fadeOut * timeScale, clipRoot.width)
        height: clipRoot.height - clipRoot.border.width * 2
        anchors.right: clipRoot.right
        anchors.top: clipRoot.top
        anchors.margins: clipRoot.border.width
        opacity: 0.3
        transform: Scale { xScale: -1; origin.x: fadeOutCanvas.width / 2}
    }
    Rectangle {
        id: fadeOutControl
        anchors.right: fadeOutCanvas.width > radius? undefined : fadeOutCanvas.right
        anchors.horizontalCenter: fadeOutCanvas.width > radius? fadeOutCanvas.left : undefined
        anchors.top: fadeOutCanvas.top
        anchors.topMargin: -10
        width: root.baseUnit * 2
        height: width
        radius: width / 2
        color: '#66FFFFFF'
        border.width: 2
        border.color: 'red'
        opacity: 0
        Drag.active: fadeOutMouseArea.drag.active
        MouseArea {
            id: fadeOutMouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.minimumX: -root.baseUnit * 2
            drag.maximumX: container.width
            property int startX
            property int startFadeOut
            onEntered: parent.opacity = 0.7
            onExited: {
                if (!pressed) {
                  parent.opacity = 0
                }
            }
            drag.smoothed: false
            onPressed: {
                root.stopScrolling = true
                startX = parent.x
                startFadeOut = clipRoot.fadeOut
                parent.anchors.right = undefined
                parent.anchors.horizontalCenter = undefined
                parent.opacity = 1
                fadeOutCanvas.opacity = 0.5
            }
            onReleased: {
                fadeOutCanvas.opacity = 0.3
                parent.opacity = 0
                root.stopScrolling = false
                if (fadeOutCanvas.width > parent.radius)
                    parent.anchors.horizontalCenter = fadeOutCanvas.left
                else
                    parent.anchors.right = fadeOutCanvas.right
                timeline.adjustFade(clipRoot.clipId, 'fadeout', clipRoot.fadeOut, startFadeOut)
                bubbleHelp.hide()
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((startX - parent.x) / timeScale)
                    if (delta != 0) {
                        var duration = Math.max(0, startFadeOut + delta)
                        duration = Math.min(duration, clipRoot.clipDuration)
                        if (clipRoot.fadeOut - 1 != duration) {
                            timeline.adjustFade(clipRoot.clipId, 'fadeout', duration, -1)
                        }
                        // Show fade duration as time in a "bubble" help.
                        var s = timeline.timecode(Math.max(duration, 0))
                        bubbleHelp.show(clipRoot.x + clipRoot.width, parentTrack.y + clipRoot.height, s)
                    }
                }
            }
        }
        SequentialAnimation on scale {
            loops: Animation.Infinite
            running: fadeOutMouseArea.containsMouse && !fadeOutMouseArea.pressed
            NumberAnimation {
                from: 1.0
                to: 0.7
                duration: 250
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                from: 0.7
                to: 1.0
                duration: 250
                easing.type: Easing.InOutQuad
            }
        }
    }

    Rectangle {
        id: trimIn
        anchors.left: clipRoot.left
        anchors.leftMargin: 0
        height: parent.height
        width: 5
        color: isAudio? 'green' : 'lawngreen'
        opacity: 0
        Drag.active: trimInMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction
        visible: root.activeTool === 0 && !mouseArea.drag.active

        MouseArea {
            id: trimInMouseArea
            anchors.fill: parent
            hoverEnabled: true
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.smoothed: false
            property bool shiftTrim: false
            property bool sizeChanged: false
            cursorShape: (containsMouse ? Qt.SizeHorCursor : Qt.ClosedHandCursor);
            onPressed: {
                root.stopScrolling = true
                clipRoot.originalX = clipRoot.x
                clipRoot.originalDuration = clipDuration
                parent.anchors.left = undefined
                shiftTrim = mouse.modifiers & Qt.ShiftModifier
                parent.opacity = 0
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.left = clipRoot.left
                if (sizeChanged) {
                    clipRoot.trimmedIn(clipRoot, shiftTrim)
                    sizeChanged = false
                }
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var delta = Math.round((trimIn.x) / timeScale)
                    if (delta !== 0) {
                        var newDuration =  clipDuration - delta
                        sizeChanged = true
                        clipRoot.trimmingIn(clipRoot, newDuration, mouse, shiftTrim)
                    }
                }
            }
            onEntered: {
                if (!pressed) {
                    parent.opacity = 0.5
                }
            }
            onExited: {
                parent.opacity = 0
            }
        }
    }
    Rectangle {
        id: trimOut
        anchors.right: clipRoot.right
        anchors.rightMargin: 0
        height: parent.height
        width: 5
        color: 'red'
        opacity: 0
        Drag.active: trimOutMouseArea.drag.active
        Drag.proposedAction: Qt.MoveAction
        visible: root.activeTool === 0 && !mouseArea.drag.active

        MouseArea {
            id: trimOutMouseArea
            anchors.fill: parent
            hoverEnabled: true
            property bool shiftTrim: false
            property bool sizeChanged: false
            cursorShape: (containsMouse ? Qt.SizeHorCursor : Qt.ClosedHandCursor);
            drag.target: parent
            drag.axis: Drag.XAxis
            drag.smoothed: false

            onPressed: {
                root.stopScrolling = true
                clipRoot.originalDuration = clipDuration
                parent.anchors.right = undefined
                shiftTrim = mouse.modifiers & Qt.ShiftModifier
                parent.opacity = 0
            }
            onReleased: {
                root.stopScrolling = false
                parent.anchors.right = clipRoot.right
                if (sizeChanged) {
                    clipRoot.trimmedOut(clipRoot, shiftTrim)
                    sizeChanged = false
                }
            }
            onPositionChanged: {
                if (mouse.buttons === Qt.LeftButton) {
                    var newDuration = Math.round((parent.x + parent.width) / timeScale)
                    if (newDuration != clipDuration) {
                        sizeChanged = true
                        clipRoot.trimmingOut(clipRoot, newDuration, mouse, shiftTrim)
                    }
                }
            }
            onEntered: {
                if (!pressed) {
                    parent.opacity = 0.5
                }
            }
            onExited: parent.opacity = 0
        }
    }

        /*MenuItem {
            id: mergeItem
            text: i18n('Merge with next clip')
            onTriggered: timeline.mergeClipWithNext(trackIndex, index, false)
        }
        MenuItem {
            text: i18n('Rebuild Audio Waveform')
            onTriggered: timeline.remakeAudioLevels(trackIndex, index)
        }*/
        /*onPopupVisibleChanged: {
            if (visible && application.OS !== 'OS X' && __popupGeometry.height > 0) {
                // Try to fix menu running off screen. This only works intermittently.
                menu.__yOffset = Math.min(0, Screen.height - (__popupGeometry.y + __popupGeometry.height + 40))
                menu.__xOffset = Math.min(0, Screen.width - (__popupGeometry.x + __popupGeometry.width))
            }
        }*/
}