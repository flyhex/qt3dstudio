/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//==============================================================================
//	Prefix
//==============================================================================
#ifndef __UICKEYDEFS_H__
#define __UICKEYDEFS_H__

enum EUICKeyDefines {
    // We start this at 0x30 so all alpha-numeric keyCodes will match Studio 1.5 Windows virtual key
    // codes
    QT3DS_KEY_0 = 0x30, // 0
    QT3DS_KEY_1, // 1
    QT3DS_KEY_2, // 2
    QT3DS_KEY_3, // 3
    QT3DS_KEY_4, // 4
    QT3DS_KEY_5, // 5
    QT3DS_KEY_6, // 6
    QT3DS_KEY_7, // 7
    QT3DS_KEY_8, // 8
    QT3DS_KEY_9, // 9

    // We start this at 0x41 so all alpha-numeric keyCodes will match Studio 1.5 Windows virtual key
    // codes
    QT3DS_KEY_A = 0x41, // A
    QT3DS_KEY_B, // B
    QT3DS_KEY_C, // C
    QT3DS_KEY_D, // D
    QT3DS_KEY_E, // E
    QT3DS_KEY_F, // F
    QT3DS_KEY_G, // G
    QT3DS_KEY_H, // H
    QT3DS_KEY_I, // I
    QT3DS_KEY_J, // J
    QT3DS_KEY_K, // K
    QT3DS_KEY_L, // L
    QT3DS_KEY_M, // M
    QT3DS_KEY_N, // N
    QT3DS_KEY_O, // O
    QT3DS_KEY_P, // P
    QT3DS_KEY_Q, // Q
    QT3DS_KEY_R, // R
    QT3DS_KEY_S, // S
    QT3DS_KEY_T, // T
    QT3DS_KEY_U, // U
    QT3DS_KEY_V, // V
    QT3DS_KEY_W, // W
    QT3DS_KEY_X, // X
    QT3DS_KEY_Y, // Y
    QT3DS_KEY_Z, // Z

    // The actual value of these items is not important
    QT3DS_KEY_BACK, // BACKSPACE key
    QT3DS_KEY_TAB, // TAB key
    QT3DS_KEY_CLEAR, // CLEAR key
    QT3DS_KEY_RETURN, // ENTER key
    QT3DS_KEY_SHIFT, // SHIFT key
    QT3DS_KEY_CONTROL, // CTRL key
    QT3DS_KEY_PAUSE, // PAUSE key
    QT3DS_KEY_CAPITAL, // CAPS LOCK key
    QT3DS_KEY_ESCAPE, // ESC key
    QT3DS_KEY_SPACE, // SPACEBAR
    QT3DS_KEY_PRIOR, // PAGE UP key
    QT3DS_KEY_NEXT, // PAGE DOWN key
    QT3DS_KEY_END, // END key
    QT3DS_KEY_HOME, // HOME key
    QT3DS_KEY_LEFT, // LEFT ARROW key
    QT3DS_KEY_UP, // UP ARROW key
    QT3DS_KEY_RIGHT, // RIGHT ARROW key
    QT3DS_KEY_DOWN, // DOWN ARROW key
    QT3DS_KEY_SELECT, // SELECT key
    QT3DS_KEY_PRINT, // PRINT key
    QT3DS_KEY_SNAPSHOT, // PRINT SCREEN key
    QT3DS_KEY_INSERT, // INS key
    QT3DS_KEY_DELETE, // DEL key
    QT3DS_KEY_HELP, // HELP key
    QT3DS_KEY_NUMPAD0, // Numeric keypad 0 key
    QT3DS_KEY_NUMPAD1, // Numeric keypad 1 key
    QT3DS_KEY_NUMPAD2, // Numeric keypad 2 key
    QT3DS_KEY_NUMPAD3, // Numeric keypad 3 key
    QT3DS_KEY_NUMPAD4, // Numeric keypad 4 key
    QT3DS_KEY_NUMPAD5, // Numeric keypad 5 key
    QT3DS_KEY_NUMPAD6, // Numeric keypad 6 key
    QT3DS_KEY_NUMPAD7, // Numeric keypad 7 key
    QT3DS_KEY_NUMPAD8, // Numeric keypad 8 key
    QT3DS_KEY_NUMPAD9, // Numeric keypad 9 key
    QT3DS_KEY_MULTIPLY, // Multiply key
    QT3DS_KEY_ADD, // Add key
    QT3DS_KEY_SEPARATOR, // Separator key
    QT3DS_KEY_SUBTRACT, // Subtract key
    QT3DS_KEY_DECIMAL, // Decimal key
    QT3DS_KEY_DIVIDE, // Divide key
    QT3DS_KEY_F1, // F1 key
    QT3DS_KEY_F2, // F2 key
    QT3DS_KEY_F3, // F3 key
    QT3DS_KEY_F4, // F4 key
    QT3DS_KEY_F5, // F5 key
    QT3DS_KEY_F6, // F6 key
    QT3DS_KEY_F7, // F7 key
    QT3DS_KEY_F8, // F8 key
    QT3DS_KEY_F9, // F9 key
    QT3DS_KEY_F10, // F10 key
    QT3DS_KEY_F11, // F11 key
    QT3DS_KEY_F12, // F12 key
    QT3DS_KEY_F13, // F13 key
    QT3DS_KEY_F14, // F14 key
    QT3DS_KEY_F15, // F15 key
    QT3DS_KEY_F16, // F16 key
    QT3DS_KEY_F17, //  F17 key
    QT3DS_KEY_F18, //  F18 key
    QT3DS_KEY_F19, //  F19 key
    QT3DS_KEY_F20, //  F20 key
    QT3DS_KEY_F21, //  F21 key
    QT3DS_KEY_F22, //  F22 key
    QT3DS_KEY_F23, //  F23 key
    QT3DS_KEY_F24, //  F24 key
    QT3DS_KEY_NUMLOCK, // NUM LOCK key
    QT3DS_KEY_SCROLL, // SCROLL LOCK key
    QT3DS_KEY_LSHIFT, // Left SHIFT key
    QT3DS_KEY_RSHIFT, // Right SHIFT key
    QT3DS_KEY_LCONTROL, // Left CONTROL key
    QT3DS_KEY_RCONTROL, // Right CONTROL key
    QT3DS_KEY_LMENU, // Left MENU key
    QT3DS_KEY_RMENU, // Right MENU key
    QT3DS_KEY_COLON_SEMICOLON, //
    QT3DS_KEY_PLUS_EQUALS, //
    QT3DS_KEY_COMMA_LT, //
    QT3DS_KEY_MINUS_UNDERSCORE, //
    QT3DS_KEY_PERIOD_GT, //
    QT3DS_KEY_FORWARDSLASH_QUESTIONMARK, //
    QT3DS_KEY_TILDE_DIACRITIC, //
    QT3DS_KEY_OPENBRACKET_OPENBRACE, //
    QT3DS_KEY_BACKWARDSLASH_SEPARATOR, //
    QT3DS_KEY_CLOSEBRACKET_CLOSEBRACE, //
    QT3DS_KEY_SINGLEQUOTE_DOUBLEQUOTE, //
};

#endif // #ifndef __UICKEYDEFS_H__
