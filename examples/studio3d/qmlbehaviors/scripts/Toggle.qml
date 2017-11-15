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
    <Property name="attribute" formalName="Attribute Name" description="Attribute on the parent element to change." animatable="False" type="String" default="rotation.x" />
    <Property name="firstValue" formalName="First Value" description="Value when not toggled." animatable="False" type="Float" default="0" />
    <Property name="secondValue" formalName="Second Value" description="Value when toggled." animatable="False" type="Float" default="1" />
    <Property name="timerInterval" formalName="Timer Interval" description="Interval for the toggles." animatable="False" type="Float" default="500" />
    <Property name="startImmediately" formalName="Start Immediately?" description="Start immediately or wait for the Start action to be called?" animatable="False" type="Boolean" default="True" />

    <Handler name="start" formalName="Start" description="Start the toggling." />
    <Handler name="stop" formalName="Stop" description="Stop the toggling." />

    <Event name="onToggle" description="Fires when toggled." />
    <Event name="onUntoggle" description="Fires when untoggled." />
]]*/

import QtStudio3D.Behavior 1.0

Qt3DSBehavior {
    //External:
    property string attribute
    property real firstValue
    property real secondValue
    property real timerInterval
    property bool startImmediately
    //Internal:
    property int index: 0
    property bool running: false
    property real timer: 0

    function start() {
        running = true;
        setAttribute(attribute, firstValue);
    }

    function stop() {
        running = false;
    }

    function toggle() {
        if (index == 0) {
            setAttribute(attribute, secondValue);
            fireEvent("onToggle");
        } else if (index == 1) {
            setAttribute(attribute, firstValue);
            fireEvent("onUntoggle");
        }
        index++;
        if (index > 1)
            index = 0;
    }

    onInitialize: {
        if (startImmediately)
            start();
    }

    onUpdate: {
        if (!running)
            return;

        timer += getDeltaTime();
        var interval = timerInterval / 1000;
        while (timer >= interval) {
            timer -= interval;
            toggle();
        }
    }
}
