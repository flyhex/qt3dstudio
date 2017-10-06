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
<Event name="onLEFTDown" category="Navigation" />
<Event name="onLEFTUp" category="Navigation" />
<Event name="onRIGHTDown" category="Navigation" />
<Event name="onRIGHTUp" category="Navigation" />
<Event name="onUPDown" category="Navigation" />
<Event name="onUPUp" category="Navigation" />
<Event name="onDOWNDown" category="Navigation" />
<Event name="onDOWNUp" category="Navigation" />
]]*/

import QtQml 2.2

QtObject {
    property var keyNames: ["NOKEY",
                            "ESCAPE",
                            "1",
                            "2",
                            "3",
                            "4",
                            "5",
                            "6",
                            "7",
                            "8",
                            "9",
                            "0",
                            "SUBTRACT",
                            "EQUALS",
                            "BACK",
                            "TAB",
                            "Q",
                            "W",
                            "E",
                            "R",
                            "T",
                            "Y",
                            "U",
                            "I",
                            "O",
                            "P",
                            "LBRACKET",
                            "RBRACKET",
                            "RETURN",
                            "LCONTROL",
                            "A",
                            "S",
                            "D",
                            "F",
                            "G",
                            "H",
                            "J",
                            "K",
                            "L",
                            "SEMICOLON",
                            "APOSTROPHE",
                            "GRAVE",
                            "LSHIFT",
                            "BACKSLASH",
                            "Z",
                            "X",
                            "C",
                            "V",
                            "B",
                            "N",
                            "M",
                            "COMMA",
                            "PERIOD",
                            "SLASH",
                            "RSHIFT",
                            "MULTIPLY",
                            "LALT",
                            "SPACE",
                            "CAPITAL",
                            "F1",
                            "F2", // 60
                            "F3",
                            "F4",
                            "F5",
                            "F6",
                            "F7",
                            "F8",
                            "F9",
                            "F10",
                            "NUMLOCK",
                            "SCROLL", // 70
                            "NUMPAD7",
                            "NUMPAD8",
                            "NUMPAD9",
                            "NUMPADSUBTRACT",
                            "NUMPAD4",
                            "NUMPAD5",
                            "NUMPAD6",
                            "NUMPADADD",
                            "NUMPAD1",
                            "NUMPAD2", // 80
                            "NUMPAD3",
                            "NUMPAD0",
                            "NUMPADDECIMAL",
                            "NOOP",
                            "ZENKAKUHANKAKU",
                            "102ND",
                            "F11",
                            "F12",
                            "F13",
                            "F14", // 90
                            "HIRAGANA",
                            "HENKAN",
                            "KATAKANAHIRAGANA",
                            "MUHENKAN",
                            "KPJPCOMMA",
                            "NUMPADENTER",
                            "RCONTROL",
                            "NUMPADDIVIDE",
                            "PRINTSCREEN",
                            "RALT", // 100
                            "LINEFEED",
                            "HOME",
                            "UP",
                            "PGUP",
                            "LEFT",
                            "RIGHT",
                            "END",
                            "DOWN",
                            "PGDN",
                            "INSERT", // 110
                            "DELETE",
                            "MACRO",
                            "MUTE",
                            "VOLUMEDOWN",
                            "VOLUMEUP",
                            "POWER",
                            "KPEQUAL",
                            "KPPLUSMINUS",
                            "PAUSE",
                            "SCALE"]

    function onInitialize() {
        Qt3ds.registerForEvent("onKeyDown", onKeyDown);
        Qt3ds.registerForEvent("onKeyUp", onKeyUp);
    }

    function onKeyDown(keyCode) {
        Qt3ds.fireEvent("on" + keyNames[keyCode] + "Down");
    }

    function onKeyUp(keyCode) {
        Qt3ds.fireEvent("on" + keyNames[keyCode] + "Up");
    }
}
