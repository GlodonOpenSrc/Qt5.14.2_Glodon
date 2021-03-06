/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.14
import QtQuick3D 1.14

Node {
    id: torus360

    property Node gizmoRoot
    property int axis: -1
    property color color: "white"
    property var space: Node.LocalSpace

    property var _axis
    property real _prevMouseAngle
    property int _draggingOnBackside
    property var _target
    property var _view

    function startDrag(targetNode, view, pointerPosition)
    {
        _target = targetNode
        _view = view

        var center = _view.mapFrom3DScene(_target.scenePosition)
        var deltaX = pointerPosition.x - center.x
        var deltaY = pointerPosition.y - center.y
        var rad = Math.atan2(deltaY, deltaX);
        _prevMouseAngle = rad * (180 / Math.PI)

        if (axis === Qt.XAxis)
            _axis = Qt.vector3d(1, 0, 0)
        else if (axis === Qt.YAxis)
            _axis = Qt.vector3d(0, 1, 0)
        else
            _axis = Qt.vector3d(0, 0, 1)

        var relCamPos = _view.camera.mapPositionToNode(torus360, Qt.vector3d(0, 0, 0));
        _draggingOnBackside = relCamPos.z > 0 ? -1 : 1
        if (_target.orientation === Node.RightHanded)
            _draggingOnBackside *= -1;
    }

    function continueDrag(pointerPosition)
    {
        var center = _view.mapFrom3DScene(_target.scenePosition)
        var deltaX = pointerPosition.x - center.x
        var deltaY = pointerPosition.y - center.y
        var rad = Math.atan2(deltaY, deltaX);
        var mouseAngle = rad * (180 / Math.PI)
        var degrees = mouseAngle - _prevMouseAngle
        degrees *= _draggingOnBackside
        _prevMouseAngle = mouseAngle

        _target.rotate(-degrees, _axis, space)
    }

    RotateGizmoTorus90 {
        id: upSegment
        pivot: Qt.vector3d(0, -1, 0)
        color: torus360.color
        gizmoAxisRoot: torus360
        pickable: true
    }

    RotateGizmoTorus90 {
        id: leftSegment
        pivot: Qt.vector3d(0, -1, 0)
        rotation: Qt.vector3d(0, 0, 90)
        color: torus360.color
        gizmoAxisRoot: torus360
        pickable: true
    }

    RotateGizmoTorus90 {
        id: bottomSegment
        pivot: Qt.vector3d(0, -1, 0)
        rotation: Qt.vector3d(0, 0, 180)
        color: torus360.color
        gizmoAxisRoot: torus360
        pickable: true
    }

    RotateGizmoTorus90 {
        id: rightSegment
        pivot: Qt.vector3d(0, -1, 0)
        rotation: Qt.vector3d(0, 0, 270)
        color: torus360.color
        gizmoAxisRoot: torus360
        pickable: true
    }

}
