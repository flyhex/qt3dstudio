/****************************************************************************
**
** Copyright (C) 2009-2011 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

class NVKeyCodeMapping
{
public:
    NVKeyCodeMapping() { memset(m_keyMapping, 0, sizeof(NVKeyCode) * QT3DS_MAX_KEYCODE); }

    void Init(JNIEnv *env, jobject thiz);

    bool MapKey(int key, NVKeyCode &code);

protected:
    void AddKeyMapping(JNIEnv *env, jobject thiz, jclass KeyCode_class, const char *name,
                       NVKeyCode value);
    NVKeyCode m_keyMapping[QT3DS_MAX_KEYCODE];
};

/* Init the mapping array, set up the event queue */
void NVKeyCodeMapping::AddKeyMapping(JNIEnv *env, jobject thiz, jclass KeyCode_class,
                                     const char *name, NVKeyCode value)
{
    // Add a new mapping...
    jfieldID id = env->GetStaticFieldID(KeyCode_class, name, "I");
    int keyID = env->GetStaticIntField(KeyCode_class, id);

    if (keyID < QT3DS_MAX_KEYCODE) {
        /* TODO TBD Should check for collision */
        m_keyMapping[keyID] = value;
    }
}

#define AddKeymappingMacro(name, value) AddKeyMapping(env, thiz, KeyCode_class, name, value)

void NVKeyCodeMapping::Init(JNIEnv *env, jobject thiz)
{
    jclass KeyCode_class = env->FindClass("android/view/KeyEvent");

    AddKeymappingMacro("KEYCODE_BACK", QT3DS_KEYCODE_BACK);
    AddKeymappingMacro("KEYCODE_TAB", QT3DS_KEYCODE_TAB);
    AddKeymappingMacro("KEYCODE_ENTER", QT3DS_KEYCODE_ENTER);

    AddKeymappingMacro("KEYCODE_SPACE", QT3DS_KEYCODE_SPACE);
    AddKeymappingMacro("KEYCODE_ENDCALL", QT3DS_KEYCODE_ENDCALL);
    AddKeymappingMacro("KEYCODE_HOME", QT3DS_KEYCODE_HOME);

    AddKeymappingMacro("KEYCODE_DPAD_LEFT", QT3DS_KEYCODE_DPAD_LEFT);
    AddKeymappingMacro("KEYCODE_DPAD_UP", QT3DS_KEYCODE_DPAD_UP);
    AddKeymappingMacro("KEYCODE_DPAD_RIGHT", QT3DS_KEYCODE_DPAD_RIGHT);
    AddKeymappingMacro("KEYCODE_DPAD_DOWN", QT3DS_KEYCODE_DPAD_DOWN);

    AddKeymappingMacro("KEYCODE_DEL", QT3DS_KEYCODE_DEL);

    AddKeymappingMacro("KEYCODE_0", QT3DS_KEYCODE_0);
    AddKeymappingMacro("KEYCODE_1", QT3DS_KEYCODE_1);
    AddKeymappingMacro("KEYCODE_2", QT3DS_KEYCODE_2);
    AddKeymappingMacro("KEYCODE_3", QT3DS_KEYCODE_3);
    AddKeymappingMacro("KEYCODE_4", QT3DS_KEYCODE_4);
    AddKeymappingMacro("KEYCODE_5", QT3DS_KEYCODE_5);
    AddKeymappingMacro("KEYCODE_6", QT3DS_KEYCODE_6);
    AddKeymappingMacro("KEYCODE_7", QT3DS_KEYCODE_7);
    AddKeymappingMacro("KEYCODE_8", QT3DS_KEYCODE_8);
    AddKeymappingMacro("KEYCODE_9", QT3DS_KEYCODE_9);

    AddKeymappingMacro("KEYCODE_A", QT3DS_KEYCODE_A);
    AddKeymappingMacro("KEYCODE_B", QT3DS_KEYCODE_B);
    AddKeymappingMacro("KEYCODE_C", QT3DS_KEYCODE_C);
    AddKeymappingMacro("KEYCODE_D", QT3DS_KEYCODE_D);
    AddKeymappingMacro("KEYCODE_E", QT3DS_KEYCODE_E);
    AddKeymappingMacro("KEYCODE_F", QT3DS_KEYCODE_F);
    AddKeymappingMacro("KEYCODE_G", QT3DS_KEYCODE_G);
    AddKeymappingMacro("KEYCODE_H", QT3DS_KEYCODE_H);
    AddKeymappingMacro("KEYCODE_I", QT3DS_KEYCODE_I);
    AddKeymappingMacro("KEYCODE_J", QT3DS_KEYCODE_J);
    AddKeymappingMacro("KEYCODE_K", QT3DS_KEYCODE_K);
    AddKeymappingMacro("KEYCODE_L", QT3DS_KEYCODE_L);
    AddKeymappingMacro("KEYCODE_M", QT3DS_KEYCODE_M);
    AddKeymappingMacro("KEYCODE_N", QT3DS_KEYCODE_N);
    AddKeymappingMacro("KEYCODE_O", QT3DS_KEYCODE_O);
    AddKeymappingMacro("KEYCODE_P", QT3DS_KEYCODE_P);
    AddKeymappingMacro("KEYCODE_Q", QT3DS_KEYCODE_Q);
    AddKeymappingMacro("KEYCODE_R", QT3DS_KEYCODE_R);
    AddKeymappingMacro("KEYCODE_S", QT3DS_KEYCODE_S);
    AddKeymappingMacro("KEYCODE_T", QT3DS_KEYCODE_T);
    AddKeymappingMacro("KEYCODE_U", QT3DS_KEYCODE_U);
    AddKeymappingMacro("KEYCODE_V", QT3DS_KEYCODE_V);
    AddKeymappingMacro("KEYCODE_W", QT3DS_KEYCODE_W);
    AddKeymappingMacro("KEYCODE_X", QT3DS_KEYCODE_X);
    AddKeymappingMacro("KEYCODE_Y", QT3DS_KEYCODE_Y);
    AddKeymappingMacro("KEYCODE_Z", QT3DS_KEYCODE_Z);

    AddKeymappingMacro("KEYCODE_STAR", QT3DS_KEYCODE_STAR);
    AddKeymappingMacro("KEYCODE_PLUS", QT3DS_KEYCODE_PLUS);
    AddKeymappingMacro("KEYCODE_MINUS", QT3DS_KEYCODE_MINUS);

    AddKeymappingMacro("KEYCODE_NUM", QT3DS_KEYCODE_NUM);

    AddKeymappingMacro("KEYCODE_ALT_LEFT", QT3DS_KEYCODE_ALT_LEFT);
    AddKeymappingMacro("KEYCODE_ALT_RIGHT", QT3DS_KEYCODE_ALT_RIGHT);

    AddKeymappingMacro("KEYCODE_SHIFT_LEFT", QT3DS_KEYCODE_SHIFT_LEFT);
    AddKeymappingMacro("KEYCODE_SHIFT_RIGHT", QT3DS_KEYCODE_SHIFT_RIGHT);

    AddKeymappingMacro("KEYCODE_APOSTROPHE", QT3DS_KEYCODE_APOSTROPHE);
    AddKeymappingMacro("KEYCODE_SEMICOLON", QT3DS_KEYCODE_SEMICOLON);
    AddKeymappingMacro("KEYCODE_EQUALS", QT3DS_KEYCODE_EQUALS);
    AddKeymappingMacro("KEYCODE_COMMA", QT3DS_KEYCODE_COMMA);
    AddKeymappingMacro("KEYCODE_PERIOD", QT3DS_KEYCODE_PERIOD);
    AddKeymappingMacro("KEYCODE_SLASH", QT3DS_KEYCODE_SLASH);
    AddKeymappingMacro("KEYCODE_GRAVE", QT3DS_KEYCODE_GRAVE);
    AddKeymappingMacro("KEYCODE_LEFT_BRACKET", QT3DS_KEYCODE_LEFT_BRACKET);
    AddKeymappingMacro("KEYCODE_BACKSLASH", QT3DS_KEYCODE_BACKSLASH);
    AddKeymappingMacro("KEYCODE_RIGHT_BRACKET", QT3DS_KEYCODE_RIGHT_BRACKET);
}

bool NVKeyCodeMapping::MapKey(int key, NVKeyCode &code)
{
    if (key < QT3DS_MAX_KEYCODE) {
        code = m_keyMapping[key];
        return true;
    } else {
        return false;
    }
}
