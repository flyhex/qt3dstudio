/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
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

/*[[
    <Property name="renderCamera" formalName="Render Camera" type="ObjectRef" default="Scene.Layer.Camera" description="The camera which will be rendering the scene." />
    <Property name="billboardType" formalName="Billboard Type" type="StringList" default="Face Camera" list="Face Camera,Match Camera Rotation"  description="Always face the camera, or only match its rotation." />
    <Property name="yOnly" formalName="Y-Axis Only?" type="Boolean" default="False"  description="Only rotate the object about the global y-axis?" />
    <Property name="startImmediately" formalName="Start Immediately?" type="Boolean" default="True" publishLevel="Advanced" description="Start immediately, or wait for the Enable action to be called?" />

    <Handler name="start" formalName="Start" category="Billboard" description="Begin keeping the parent object billboarded." />
    <Handler name="stop" formalName="Stop" category="Billboard" description="Stop rotating the parent object." />
]]*/

import QtQml 2.2

QtObject {
    //External:
    property string renderCamera
    property string billboardType
    property bool yOnly
    property bool startImmediately
    //Internal:
    property bool running: false
    property var updateFunction

    function start() {
        running = true;
    }

    function stop() {
        running = false;
    }

    function onInitialize() {
        if (billboardType === "Face Camera") {
            if (!yOnly)
                updateFunction = faceCamera;
            else
                updateFunction = faceCameraGlobalY;
        }
        else {
            if (!yOnly)
                updateFunction = matchRotation;
            else
                updateFunction = matchRotationGlobalY;
        }

        if (startImmediately)
            start();
    }

    function onUpdate() {
        if (!running)
            return;

        updateFunction();
    }

    function faceCamera() {
        var cameraTransform = Qt3DSRuntime.calculateGlobalTransform(renderCamera);
        var cameraSpot = cameraTransform.row(3).toVector3d();
        var myTransform = Qt3DSRuntime.calculateGlobalTransform();
        var mySpot = myTransform.row(3).toVector3d();

        var matrix = Qt3DSRuntime.calculateGlobalTransform(Qt3DSRuntime.getParent()).inverted();
        matrix.m41 = 0;
        matrix.m42 = 0;
        matrix.m43 = 0;

        var rotateRay = cameraSpot
                        .minus(mySpot)
                        .times(matrix);

        var rotation = Qt3DSRuntime.lookAt(rotateRay);
        setAttributeVector("rotation", rotation);
    }

    function faceCameraGlobalY() {
        var cameraTransform = Qt3DSRuntime.calculateGlobalTransform(renderCamera);
        var cameraSpot = cameraTransform.row(3).toVector3d();
        var myTransform = Qt3DSRuntime.calculateGlobalTransform();
        var mySpot = myTransform.row(3).toVector3d();

        var rotateRay = cameraSpot.minus(mySpot);

        var rotation = getAttributeVector("rotation");
        rotation.y = Math.atan2(rotateRay.x, rotateRay.z);
        setAttributeVector("rotation", rotation);
    }

    function matchRotation() {
        var cameraTransform = Qt3DSRuntime.calculateGlobalTransform(renderCamera);
        var cameraSpot = cameraTransform.row(3).toVector3d();

        var matrix = Qt3DSRuntime.calculateGlobalTransform(Qt3DSRuntime.getParent()).inverted();
        matrix.m41 = 0;
        matrix.m42 = 0;
        matrix.m43 = 0;

        var rotateRay = Qt.vector3d(0, 0, 1)
                        .times(cameraTransform)
                        .minus(cameraSpot)
                        .times(matrix);

        var rotation = Qt3DSRuntime.lookAt(rotateRay);
        setAttributeVector("rotation", rotation);
    }

    function matchRotationGlobalY() {
        var cameraTransform = Qt3DSRuntime.calculateGlobalTransform(renderCamera);
        var cameraSpot = cameraTransform.row(3).toVector3d();

        var rotateRay = Qt.vector3d(0, 0, 1)
                        .times(cameraTransform)
                        .minus(cameraSpot)

        var rotation = getAttributeVector("rotation");
        rotation.y = Qt3DSRuntime.lookAt(rotateRay).y;
        setAttributeVector("rotation", rotation);
    }

    function getAttributeVector(name) {
        var vec = Qt.vector3d(0, 0, 0);
        Qt3DSRuntime.getAttribute(name + ".x", vec.x);
        Qt3DSRuntime.getAttribute(name + ".y", vec.y);
        Qt3DSRuntime.getAttribute(name + ".z", vec.z);
        return vec;
    }

    function setAttributeVector(name, vec) {
        Qt3DSRuntime.setAttribute(name + ".x", vec.x);
        Qt3DSRuntime.setAttribute(name + ".y", vec.y);
        Qt3DSRuntime.setAttribute(name + ".z", vec.z);
    }
}
