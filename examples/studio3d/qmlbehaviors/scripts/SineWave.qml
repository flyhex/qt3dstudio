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
    <Property name="attribute"            formalName="Attribute Name"     description="Attribute on the parent element to change."                   animatable="False" type="String"  default="rotation.x" />
    <Property name="ampOffset"            formalName="Center Value"       description="Value to center around."                                      animatable="False" type="Float"   default="0"          />
    <Property name="amplitude"            formalName="Amplitude"          description="Maximum value added to/subtracted from the center."           animatable="True" type="Float"    default="50"         />
    <Property name="period"               formalName="Period"             description="Number of seconds to complete one full cycle."                animatable="False" type="Float"   default="5"          />
    <Property name="timeOffset"           formalName="Start Time Offset"  description="Start the sine wave at a different point."                    animatable="False" type="Float"   default="0"  />
    <Property name="startImmediately"     formalName="Start Immediately?" description="Start immediately or wait for the Start action to be called?" animatable="False" type="Boolean" default="True"       />

    <Handler name="start"  formalName="Start"  description="Start the Sine Wave control of the property."              />
    <Handler name="stop"   formalName="Stop"   description="Stop the Sine Wave control of the property."               />
    <Handler name="toggle" formalName="Toggle" description="If the behavior is running, stop it; otherwise, start it." />

    <Event name="onStarted" description="Fires when the Sine Wave is started." />
    <Event name="onStopped" description="Fires when the Sine Wave is stopped." />
]]*/

import QtQml 2.2

QtObject {
    //External:
    property string attribute
    property real ampOffset
    property real amplitude
    property real period
    property real timeOffset
    property bool startImmediately
    //Internal:
    property bool running: false
    property bool inDegrees: false
    property real elapsedTime: 0

    function start() {
        if (!running) {
            running = true;
            Qt3ds.fireEvent("onStarted");
        }
    }

    function stop() {
        if (running) {
            running = false;
            Qt3ds.fireEvent("onStopped");
        }
    }

    function toggle() {
        if (running)
            stop();
        else
            start();
    }

    function onInitialize() {
        if (attribute.indexOf("rotation") != -1)
            inDegrees = true;

        if (startImmediately)
            start();
    }

    function onUpdate() {
        if (!running)
            return;

        if (inDegrees) {
            ampOffset *= (Math.PI / 180);
            amplitude *= (Math.PI / 180);
        }

        elapsedTime += Qt3ds.getDeltaTime();
        var value = ampOffset + amplitude * Math.cos(elapsedTime * Math.PI * 2 / period);
        Qt3ds.setAttribute(attribute, value);
    }
}
