/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#ifndef INCLUDED_HOT_KEYS_H
#define INCLUDED_HOT_KEYS_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSString.h"
#include "Qt3DSObjectCounter.h"

#include <QtGlobal>

// New QAction based shortcuts can be added with this macro
#define ADD_GLOBAL_SHORTCUT(actionParent, keyList, handlerFunc) \
{ \
    QAction *action = new QAction(QStringLiteral(#handlerFunc), actionParent); \
    QObject::connect(action, &QAction::triggered, this, &handlerFunc); \
    action->setShortcuts(QList<QKeySequence>() << keyList); \
    actionParent->addAction(action); \
}

class KeyEventFilter;

//=============================================================================
/**
 * Pure-Virtual class for destination of key events.
 */
//=============================================================================
class CHotKeyConsumer
{
public:
    virtual ~CHotKeyConsumer() = default;
    virtual bool OnHotKeyEvent(int inChar, int inRepeatCount,
                               Qt::KeyboardModifiers modifiers) = 0;
};

//=============================================================================
/**
 * Dynamic implementation of CHotKeyConsumer to route events to functions.
 */
//=============================================================================
template <class Fixture>
class CDynHotKeyConsumer : public CHotKeyConsumer
{
    typedef void (Fixture::*TEventMethod)();
    typedef bool (Fixture::*TSmartEventMethod)(int inChar, int inRepeatCount,
                                               Qt::KeyboardModifiers modifiers);
    typedef bool (Fixture::*TBoolEventMethod)();
    typedef int (Fixture::*TIntEventMethod)();

public:
    //==========================================================================
    /**
     * Create a HotKeyConsumer mapping the event to a function with no arguments.
     *
     * @param inObject the object the method is to be called on.
     * @param inEventMethod the method that is to be called.
     */
    //==========================================================================
    CDynHotKeyConsumer(Fixture *inObject, TEventMethod inEventMethod)
    {
        m_Object = inObject;
        m_Method = inEventMethod;
        m_SmartMethod = NULL;
        m_BoolMethod = NULL;
        m_IntMethod = NULL;
    }

    //==========================================================================
    /**
     * Create a HotKeyConsumer mapping the event to a function with normal key args.
     *
     * @param inObject the object the method is to be called on.
     * @param inEventMethod the method that is to be called.
     */
    //==========================================================================
    CDynHotKeyConsumer(Fixture *inObject, TSmartEventMethod inEventMethod)
    {
        m_Object = inObject;
        m_Method = NULL;
        m_SmartMethod = inEventMethod;
        m_BoolMethod = NULL;
        m_IntMethod = NULL;
    }

    CDynHotKeyConsumer(Fixture *inObject, TBoolEventMethod inEventMethod)
    {
        m_Object = inObject;
        m_Method = NULL;
        m_SmartMethod = NULL;
        m_BoolMethod = inEventMethod;
        m_IntMethod = NULL;
    }

    //==========================================================================
    /**
     * Create a HotKeyConsumer mapping the event to a function with normal key args.
     *
     * @param inObject the object the method is to be called on.
     * @param inEventMethod the method that is to be called.
     */
    //==========================================================================
    CDynHotKeyConsumer(Fixture *inObject, TIntEventMethod inEventMethod)
    {
        m_Object = inObject;
        m_Method = NULL;
        m_SmartMethod = NULL;
        m_BoolMethod = NULL;
        m_IntMethod = inEventMethod;
    }

    //==========================================================================
    /**
     * Copy constructor.
     */
    //==========================================================================
    CDynHotKeyConsumer(const CDynHotKeyConsumer &inConsumer)
    {
        m_Object = inConsumer.m_Object;
        m_Method = inConsumer.m_Method;
        m_SmartMethod = inConsumer.m_SmartMethod;
    }

    //==========================================================================
    /**
     * This gets called on a registered key event, and in turn calls the function
     * that this was created with.
     *
     * @see WM_KEYDOWN for argument descriptions.
     */
    //==========================================================================
    bool OnHotKeyEvent(int inChar, int inRepeatCount,
                               Qt::KeyboardModifiers modifiers) override
    {
        bool theRetVal = true;

        // Determine which method to call...
        if (m_Method != NULL)
            (m_Object->*m_Method)();
        else if (m_SmartMethod != NULL)
            theRetVal = (m_Object->*m_SmartMethod)(inChar, inRepeatCount, modifiers);
        else if (m_BoolMethod != NULL)
            theRetVal = (m_Object->*m_BoolMethod)();
        else
            (m_Object->*m_IntMethod)();

        return theRetVal;
    }

protected:
    Fixture *m_Object;
    TEventMethod m_Method;
    TBoolEventMethod m_BoolMethod;
    TIntEventMethod m_IntMethod;
    TSmartEventMethod m_SmartMethod;
};

class CHotKeys
{
public:
    enum Modifier {

        MODIFIER_CONTROL = Qt::ControlModifier,
        MODIFIER_SHIFT = Qt::ShiftModifier,
        MODIFIER_ALT = Qt::AltModifier,

        MOUSE_LBUTTON = 0x100000,
        MOUSE_MBUTTON = 0x200000,
        MOUSE_RBUTTON = 0x400000
    };
    Q_DECLARE_FLAGS(Modifiers, Modifier)

public:
    CHotKeys();
    ~CHotKeys();

    DEFINE_OBJECT_COUNTER(CHotKeys)

    void RegisterKeyEvent(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                          int inCharacter);
    void RegisterKeyDownEvent(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                              int inVirtualKey);
    void RegisterKeyUpEvent(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                            int inVirtualKey);

    void RegisterKeyEvent(CHotKeyConsumer *inConsumer, const Q3DStudio::CString &inString);
    void RegisterKeyDownEvent(CHotKeyConsumer *inConsumer, const Q3DStudio::CString &inString);
    void RegisterKeyUpEvent(CHotKeyConsumer *inConsumer, const Q3DStudio::CString &inString);

    void UnregisterKeyEvent(Qt::KeyboardModifiers inModifiers, int inCharacter);
    void UnregisterKeyDownEvent(Qt::KeyboardModifiers inModifiers, int inVirtualKey);
    void UnregisterKeyUpEvent(Qt::KeyboardModifiers inModifiers, int inVirtualKey);

    void UnregisterKeyEvent(const Q3DStudio::CString &inString);
    void UnregisterKeyDownEvent(const Q3DStudio::CString &inString);
    void UnregisterKeyUpEvent(const Q3DStudio::CString &inString);

    bool OnChar(int inChar, int inRepeatCount, Qt::KeyboardModifiers modifiers);
    bool OnKeyDown(int inChar, int inRepeatCount, Qt::KeyboardModifiers modifiers);
    bool OnKeyUp(int inChar, int inRepeatCount, Qt::KeyboardModifiers modifiers);

    void ClearAllEvents();

    void CaptureHotKeys();
    void ReleaseHotKeys();

    static bool IsKeyDown(Qt::KeyboardModifier inKeyCode);
    static Qt::KeyboardModifiers GetCurrentKeyModifiers();
    static bool isFocusOnControlThatWantsKeys();

protected:
    struct SHotKeyInfo
    {
        CHotKeyConsumer *Consumer;
        int Character;
        Qt::KeyboardModifiers Modifiers;
    };

    typedef std::vector<SHotKeyInfo *> THotKeyList;
    typedef std::map<unsigned int, THotKeyList> THotKeyMap;

    THotKeyMap m_KeyCharMap;
    THotKeyMap m_KeyDownMap;
    THotKeyMap m_KeyUpMap;
    KeyEventFilter *m_EventFilter;
    bool m_CaptureKeys; ///< Boolean that ignores all non-modified keys

    void InsertConsumer(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                        int inCharacter, THotKeyMap *inMap);
    void RemoveConsumers(Qt::KeyboardModifiers inModifiers, int inCharacter, THotKeyMap *inMap);
    bool FireEvent(int inChar, int inRepeatCount, Qt::KeyboardModifiers modifiers,
                   THotKeyMap *inMap);
    Qt::KeyboardModifier GetModifierFromString(const Q3DStudio::CString &inString);
    int GetKeyFromString(Q3DStudio::CString inChar);
    void GetModAndKeyFromString(const Q3DStudio::CString &inString, Qt::KeyboardModifiers &inMod,
                                int &inKey);
};
#endif // INCLUDED_HOT_KEYS_H
