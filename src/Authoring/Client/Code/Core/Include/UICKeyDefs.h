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
    UIC_KEY_0 = 0x30, // 0
    UIC_KEY_1, // 1
    UIC_KEY_2, // 2
    UIC_KEY_3, // 3
    UIC_KEY_4, // 4
    UIC_KEY_5, // 5
    UIC_KEY_6, // 6
    UIC_KEY_7, // 7
    UIC_KEY_8, // 8
    UIC_KEY_9, // 9

    // We start this at 0x41 so all alpha-numeric keyCodes will match Studio 1.5 Windows virtual key
    // codes
    UIC_KEY_A = 0x41, // A
    UIC_KEY_B, // B
    UIC_KEY_C, // C
    UIC_KEY_D, // D
    UIC_KEY_E, // E
    UIC_KEY_F, // F
    UIC_KEY_G, // G
    UIC_KEY_H, // H
    UIC_KEY_I, // I
    UIC_KEY_J, // J
    UIC_KEY_K, // K
    UIC_KEY_L, // L
    UIC_KEY_M, // M
    UIC_KEY_N, // N
    UIC_KEY_O, // O
    UIC_KEY_P, // P
    UIC_KEY_Q, // Q
    UIC_KEY_R, // R
    UIC_KEY_S, // S
    UIC_KEY_T, // T
    UIC_KEY_U, // U
    UIC_KEY_V, // V
    UIC_KEY_W, // W
    UIC_KEY_X, // X
    UIC_KEY_Y, // Y
    UIC_KEY_Z, // Z

    // The actual value of these items is not important
    UIC_KEY_BACK, // BACKSPACE key
    UIC_KEY_TAB, // TAB key
    UIC_KEY_CLEAR, // CLEAR key
    UIC_KEY_RETURN, // ENTER key
    UIC_KEY_SHIFT, // SHIFT key
    UIC_KEY_CONTROL, // CTRL key
    UIC_KEY_PAUSE, // PAUSE key
    UIC_KEY_CAPITAL, // CAPS LOCK key
    UIC_KEY_ESCAPE, // ESC key
    UIC_KEY_SPACE, // SPACEBAR
    UIC_KEY_PRIOR, // PAGE UP key
    UIC_KEY_NEXT, // PAGE DOWN key
    UIC_KEY_END, // END key
    UIC_KEY_HOME, // HOME key
    UIC_KEY_LEFT, // LEFT ARROW key
    UIC_KEY_UP, // UP ARROW key
    UIC_KEY_RIGHT, // RIGHT ARROW key
    UIC_KEY_DOWN, // DOWN ARROW key
    UIC_KEY_SELECT, // SELECT key
    UIC_KEY_PRINT, // PRINT key
    UIC_KEY_SNAPSHOT, // PRINT SCREEN key
    UIC_KEY_INSERT, // INS key
    UIC_KEY_DELETE, // DEL key
    UIC_KEY_HELP, // HELP key
    UIC_KEY_NUMPAD0, // Numeric keypad 0 key
    UIC_KEY_NUMPAD1, // Numeric keypad 1 key
    UIC_KEY_NUMPAD2, // Numeric keypad 2 key
    UIC_KEY_NUMPAD3, // Numeric keypad 3 key
    UIC_KEY_NUMPAD4, // Numeric keypad 4 key
    UIC_KEY_NUMPAD5, // Numeric keypad 5 key
    UIC_KEY_NUMPAD6, // Numeric keypad 6 key
    UIC_KEY_NUMPAD7, // Numeric keypad 7 key
    UIC_KEY_NUMPAD8, // Numeric keypad 8 key
    UIC_KEY_NUMPAD9, // Numeric keypad 9 key
    UIC_KEY_MULTIPLY, // Multiply key
    UIC_KEY_ADD, // Add key
    UIC_KEY_SEPARATOR, // Separator key
    UIC_KEY_SUBTRACT, // Subtract key
    UIC_KEY_DECIMAL, // Decimal key
    UIC_KEY_DIVIDE, // Divide key
    UIC_KEY_F1, // F1 key
    UIC_KEY_F2, // F2 key
    UIC_KEY_F3, // F3 key
    UIC_KEY_F4, // F4 key
    UIC_KEY_F5, // F5 key
    UIC_KEY_F6, // F6 key
    UIC_KEY_F7, // F7 key
    UIC_KEY_F8, // F8 key
    UIC_KEY_F9, // F9 key
    UIC_KEY_F10, // F10 key
    UIC_KEY_F11, // F11 key
    UIC_KEY_F12, // F12 key
    UIC_KEY_F13, // F13 key
    UIC_KEY_F14, // F14 key
    UIC_KEY_F15, // F15 key
    UIC_KEY_F16, // F16 key
    UIC_KEY_F17, //  F17 key
    UIC_KEY_F18, //  F18 key
    UIC_KEY_F19, //  F19 key
    UIC_KEY_F20, //  F20 key
    UIC_KEY_F21, //  F21 key
    UIC_KEY_F22, //  F22 key
    UIC_KEY_F23, //  F23 key
    UIC_KEY_F24, //  F24 key
    UIC_KEY_NUMLOCK, // NUM LOCK key
    UIC_KEY_SCROLL, // SCROLL LOCK key
    UIC_KEY_LSHIFT, // Left SHIFT key
    UIC_KEY_RSHIFT, // Right SHIFT key
    UIC_KEY_LCONTROL, // Left CONTROL key
    UIC_KEY_RCONTROL, // Right CONTROL key
    UIC_KEY_LMENU, // Left MENU key
    UIC_KEY_RMENU, // Right MENU key
    UIC_KEY_COLON_SEMICOLON, //
    UIC_KEY_PLUS_EQUALS, //
    UIC_KEY_COMMA_LT, //
    UIC_KEY_MINUS_UNDERSCORE, //
    UIC_KEY_PERIOD_GT, //
    UIC_KEY_FORWARDSLASH_QUESTIONMARK, //
    UIC_KEY_TILDE_DIACRITIC, //
    UIC_KEY_OPENBRACKET_OPENBRACE, //
    UIC_KEY_BACKWARDSLASH_SEPARATOR, //
    UIC_KEY_CLOSEBRACKET_CLOSEBRACE, //
    UIC_KEY_SINGLEQUOTE_DOUBLEQUOTE, //
};

#endif // #ifndef __UICKEYDEFS_H__
