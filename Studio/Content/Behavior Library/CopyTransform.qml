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
    <Property name="targetObject" formalName="Copy Target" type="ObjectRef" default="Scene.Layer.Camera" description="Object, which transform will be copied" />
    <Property name="offsetMode" formalName="Additive mode" type="Boolean" default="True"  description="Add target transform to this transform" />
    <Property name="posMode" formalName="Position" type="Boolean" default="False"  description="Copy position of target" />
    <Property name="posX" formalName="X" type="Boolean" default="True"  description="X position of target">
        <ShowIfEqual property="posMode" value="True" />
    </Property>
    <Property name="posY" formalName="Y" type="Boolean" default="True" description="Y position of target">
        <ShowIfEqual property="posMode" value="True" />
    </Property>
    <Property name="posZ" formalName="Z" type="Boolean" default="True"  description="Z position of target">
        <ShowIfEqual property="posMode" value="True" />
    </Property>
    <Property name="rotMode" formalName="Rotation" type="Boolean" default="False"  description="Copy rotation of target" />
    <Property name="rotX" formalName="X" type="Boolean" default="True"  description="X rotation of target">
        <ShowIfEqual property="rotMode" value="True" />
    </Property>
    <Property name="rotY" formalName="Y" type="Boolean" default="True"  description="Y rotation of target">
        <ShowIfEqual property="rotMode" value="True" />
    </Property>
    <Property name="rotZ" formalName="Z" type="Boolean" default="True"  description="Z rotation of target">
        <ShowIfEqual property="rotMode" value="True" />
    </Property>
    <Property name="sclMode" formalName="Scale" type="Boolean" default="False"  description="Copy scale of target" />
    <Property name="sclX" formalName="X" type="Boolean" default="True"  description="X scale of target">
        <ShowIfEqual property="sclMode" value="True" />
    </Property>
    <Property name="sclY" formalName="Y" type="Boolean" default="True"  description="Y scale of target">
        <ShowIfEqual property="sclMode" value="True" />
    </Property>
    <Property name="sclZ" formalName="Z" type="Boolean" default="True"  description="Z scale of target">
        <ShowIfEqual property="sclMode" value="True" />
    </Property>
    <Property name="startImmediately" formalName="Start Immediately?" type="Boolean" default="True" publishLevel="Advanced" description="Start immediately, or wait for\nthe Enable action to be called?" />
    <Handler name="start" formalName="Start" category="CopyTransform" description="Begin copying the transform" />
    <Handler name="stop" formalName="Stop" category="CopyTransform" description="Stop copying the transform" />
]]*/

import QtStudio3D.Behavior 1.0

Behavior {
    //External:
    property string targetObject
    property bool offsetMode
    property bool startImmediately
    property bool posMode
    property bool posX
    property bool posY
    property bool posZ
    property bool rotMode
    property bool rotX
    property bool rotY
    property bool rotZ
    property bool sclMode
    property bool sclX
    property bool sclY
    property bool sclZ
    //Internal:
    property bool running: false
    property var updateFunction
    property var offsetAmount
    property var startRotation
    property var startScale
    property var startPosition
    property var targetStartScale
    property var scaleRatio

    function start() {
        running = true;
    }
    function stop() {
        running = false;
    }
    onInitialize: {
        var targetTransform = calculateGlobalTransform(targetObject);
        var position = targetTransform.row(3).toVector3d();
        var thisTransform = calculateGlobalTransform();
        var startRotMatrix = transformToRotationMatrix(thisTransform);
        startScale = calculateScale(thisTransform);
        startRotation = matrixToEuler(startRotMatrix);
        startPosition = thisTransform.row(3).toVector3d();
        targetStartScale = calculateScale(targetTransform);
        offsetAmount = startPosition.minus(position);

        var startx = startScale.x/targetStartScale.x;
        var starty = startScale.y/targetStartScale.y;
        var startz = startScale.z/targetStartScale.z;

        scaleRatio = Qt.vector3d(startx,starty,startz);

        if (offsetMode) updateFunction = copyTransformOffset;
        else updateFunction = copyTransformParent;
        if (startImmediately) start();
    }
    onUpdate: {
        if (!running)
            return;
        updateFunction();
    }
    function copyTransformParent(){
        var thisTransform = calculateGlobalTransform();
        var thisPosition = thisTransform.row(3).toVector3d();
        var targetTransform = calculateGlobalTransform(targetObject);
        var targetPosition = targetTransform.row(3).toVector3d();
        if (sclMode) {
            var targetScale = calculateScale(targetTransform);
            var resultscale = targetScale;
            if (!sclX) resultscale.x = startScale.x;
            if (!sclY) resultscale.y = startScale.y;
            if (!sclZ) resultscale.z = startScale.z;
            setAttributeVector("scale", resultscale);
        }
        if (rotMode) {
            var thisRotation = matrixToEuler(transformToRotationMatrix((thisTransform)));
            var targetRotation = matrixToEuler(transformToRotationMatrix(targetTransform));
            var resultrotation = targetRotation;
            if (!rotX) resultrotation.x = startRotation.x;
            if (!rotY) resultrotation.y = startRotation.y;
            if (!rotZ) resultrotation.z = startRotation.z;
            setAttributeVector("rotation",resultrotation );
        }
        if (posMode) {
            var resultposition = targetPosition;
            if (!posX) resultposition.x = startPosition.x;
            if (!posY) resultposition.y = startPosition.y;
            if (!posZ) resultposition.z = startPosition.z;
            setAttributeVector("position", resultposition);
            console.log(resultposition.x);
        }
    }
    function copyTransformOffset(){
        var thisTransform = calculateGlobalTransform();
        var thisPosition = thisTransform.row(3).toVector3d();
        var targetTransform = calculateGlobalTransform(targetObject);
        var targetPosition = targetTransform.row(3).toVector3d();
        if (sclMode) {
            var targetScale = calculateScale(targetTransform);
            var scaleOffset = targetScale.minus(targetStartScale);
            var resultscale = startScale.plus(scaleOffset.times(scaleRatio));
            if (!sclX) resultscale.x = startScale.x;
            if (!sclY) resultscale.y = startScale.y;
            if (!sclZ) resultscale.z = startScale.z;
            setAttributeVector("scale", resultscale);
        }
        if (rotMode) {
            var targetRotation = matrixToEuler(transformToRotationMatrix(targetTransform));
            var resultrotation = startRotation.plus(targetRotation);
            if (!rotX) resultrotation.x = startRotation.x;
            if (!rotY) resultrotation.y = startRotation.y;
            if (!rotZ) resultrotation.z = startRotation.z;
            setAttributeVector("rotation",resultrotation );
        }
        if (posMode) {
            var resultposition = targetPosition.plus(offsetAmount);
            if (!posX) resultposition.x = startPosition.x;
            if (!posY) resultposition.y = startPosition.y;
            if (!posZ) resultposition.z = startPosition.z;
            setAttributeVector("position", resultposition);
        }
    }
    function calculateScale(transForm){
        var VecScaleX = transForm.row(0).toVector3d();
        var TargetX = VecScaleX.length();
        var VecScaleY = transForm.row(1).toVector3d();
        var TargetY = VecScaleY.length();
        var VecScaleZ = transForm.row(2).toVector3d();
        var TargetZ = VecScaleZ.length();
        var result = Qt.vector3d(TargetX,TargetY,TargetZ);
        return result;
    }
    function transformToRotationMatrix(transForm){
        var rotationMatrix = transForm;
        var VecScaleX = transForm.row(0).toVector3d();
        var TargetX = VecScaleX.length();
        var VecScaleY = transForm.row(1).toVector3d();
        var TargetY = VecScaleY.length();
        var VecScaleZ = transForm.row(2).toVector3d();
        var TargetZ = VecScaleZ.length();
        var result = Qt.vector3d(TargetX,TargetY,TargetZ);

        rotationMatrix.m11 = transForm.m11/TargetX;
        rotationMatrix.m12 = transForm.m12/TargetX;
        rotationMatrix.m13 = transForm.m13/TargetX;
        rotationMatrix.m21 = transForm.m21/TargetY;
        rotationMatrix.m22 = transForm.m22/TargetY;
        rotationMatrix.m23 = transForm.m23/TargetY;
        rotationMatrix.m31 = transForm.m31/TargetZ;
        rotationMatrix.m32 = transForm.m32/TargetZ;
        rotationMatrix.m33 = transForm.m33/TargetZ;
        return rotationMatrix;
    }
    function getAttributeVector(name) {
        var vec = Qt.vector3d(0, 0, 0);
        getAttribute(name + ".x", vec.x);
        getAttribute(name + ".y", vec.y);
        getAttribute(name + ".z", vec.z);
        return vec;
    }
    function setAttributeVector(name, vec) {
        setAttribute(name + ".x", vec.x);
        setAttribute(name + ".y", vec.y);
        setAttribute(name + ".z", vec.z);
    }
}
