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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "HotKeys.h"
#include "StringTokenizer.h"

#include "Studio/_Win/UI/SceneView.h"
#include "Studio/MainFrm.h"
#include "Studio/Controls/TextEdit.h"
#include "Studio/Controls/WidgetControl.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qevent.h>
#include <QtQuickWidgets/qquickwidget.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>
#include <QtWidgets/qcombobox.h>

class KeyEventFilter : public QObject
{
public:
    KeyEventFilter(CHotKeys* hotkeys)
        : QObject(qApp)
        , m_hotkeys(hotkeys)
    {
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        const QEvent::Type eventType = event->type();
        if (eventType != QEvent::ShortcutOverride
                && eventType != QEvent::KeyPress
                && eventType != QEvent::KeyRelease) {
            return false;
        }

        QKeyEvent *ke = static_cast<QKeyEvent *>(event);

        if (eventType == QEvent::ShortcutOverride) {
            // If we are in key consuming control, eat all plain and shift-adjusted hotkeys
            // We want to also skip editing related global hotkeys (CTRL-A/X/C/V/Z)
            // and value adjusting keys (CTRL-UP/DOWN).
            const bool normalChar = !(ke->modifiers() && ke->modifiers() != Qt::ShiftModifier);
            const bool editShortcut = (ke->modifiers() == Qt::ControlModifier)
                    && (ke->key() == Qt::Key_C || ke->key() == Qt::Key_V || ke->key() == Qt::Key_Z
                        || ke->key() == Qt::Key_X || ke->key() == Qt::Key_A
                        || ke->key() == Qt::Key_Up || ke->key() == Qt::Key_Down);
            if ((normalChar || editShortcut) && m_hotkeys->isFocusOnControlThatWantsKeys()) {
                ke->accept();
                return true;
            }
        }

        if (qobject_cast<CSceneView *>(watched) == nullptr
                && qobject_cast<CMainFrame *>(watched) == nullptr
                && qobject_cast<QComboBox *>(watched) == nullptr
                && qobject_cast<CPlayerContainerWnd *>(watched) == nullptr) {
            return false;
        }

        // Ignore global shortcuts if we are in an "old" style textedit or qml control that
        // wants key events.
        if (m_hotkeys->isFocusOnControlThatWantsKeys())
            return false;

        switch (eventType) {
        case QEvent::ShortcutOverride:
        case QEvent::KeyPress: {
                bool success = m_hotkeys->OnKeyDown(ke->key(), ke->count(), ke->modifiers())
                        || m_hotkeys->OnChar(ke->key(), ke->count(), ke->modifiers());
                if (success)
                    ke->accept();
                return success;
        }
        case QEvent::KeyRelease: {
            // We are not in a text field, so we can
            // ignore auto-repeated key releases as it breaks
            // timeline animation on/off switching by hotkey
            if (!ke->isAutoRepeat())
                return m_hotkeys->OnKeyUp(ke->key(), ke->count(), ke->modifiers());
            else
                return true;
        }
        default:
            break;
        }
        return false;
    }

private:
    CHotKeys* m_hotkeys;
};

//==============================================================================
//	Defines
//==============================================================================

IMPLEMENT_OBJECT_COUNTER(CHotKeys)

//=============================================================================
/**
 * Constructor, nothing interesting.
 */
//=============================================================================
CHotKeys::CHotKeys()
    : m_CaptureKeys(false)
{
    ADDTO_OBJECT_COUNTER(CHotKeys)

    m_EventFilter = new KeyEventFilter(this);
    qApp->installEventFilter(m_EventFilter);
}

//=============================================================================
/**
 * Destructor, have to delete all the listeners.
 * This is not the cleanest way to do this but, oh well.
 */
//=============================================================================
CHotKeys::~CHotKeys()
{
    if (qApp)
        qApp->removeEventFilter(m_EventFilter);
    this->ClearAllEvents();
    REMOVEFROM_OBJECT_COUNTER(CHotKeys)
}

//=============================================================================
/**
 * Clears and deletes all the events from this.
 */
//=============================================================================
void CHotKeys::ClearAllEvents()
{
    // Delete all the OnChar listeners
    THotKeyMap::iterator theMapPos = m_KeyCharMap.begin();
    for (; theMapPos != m_KeyCharMap.end(); ++theMapPos) {
        THotKeyList::iterator theListPos = theMapPos->second.begin();
        for (; theListPos != theMapPos->second.end(); ++theListPos) {
            SHotKeyInfo *theInfo = (*theListPos);
            delete theInfo->Consumer;
            delete theInfo;
        }
    }

    m_KeyCharMap.clear();

    // Delete all the OnKeyDown listeners.
    theMapPos = m_KeyDownMap.begin();
    for (; theMapPos != m_KeyDownMap.end(); ++theMapPos) {
        THotKeyList::iterator theListPos = theMapPos->second.begin();
        for (; theListPos != theMapPos->second.end(); ++theListPos) {
            SHotKeyInfo *theInfo = (*theListPos);
            delete theInfo->Consumer;
            delete theInfo;
        }
    }

    m_KeyDownMap.clear();

    // Delete all the OnKeyUp listeners.
    theMapPos = m_KeyUpMap.begin();
    for (; theMapPos != m_KeyUpMap.end(); ++theMapPos) {
        THotKeyList::iterator theListPos = theMapPos->second.begin();
        for (; theListPos != theMapPos->second.end(); ++theListPos) {
            SHotKeyInfo *theInfo = (*theListPos);
            delete theInfo->Consumer;
            delete theInfo;
        }
    }

    m_KeyUpMap.clear();
}

//=============================================================================
/**
 * Register a consumer to get called on the equivalent of OnChar.
 * This will call the consumer when all the modifiers are met and the character
 * has been received. If a modifier such as MOD_SHIFT is set but the key is not
 * pressed then the consumer will not be called.
 *
 * If there are duplicate listeners for the same exact event the previous one
 * will be removed.
 *
 * @param inConsumer the consumer to add for events, this will be deleted by this.
 * @param inModifiers mask of keyboard modifiers- MOD_SHIFT, MOD_CONTROL, MOD_MENU...
 * @param inCharacter the character the event is to occur on.
 */
//=============================================================================
void CHotKeys::RegisterKeyEvent(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                                int inCharacter)
{
    // Remove any previous consumers.
    this->RemoveConsumers(inModifiers, inCharacter, &m_KeyCharMap);
    this->InsertConsumer(inConsumer, inModifiers, inCharacter, &m_KeyCharMap);
}

//=============================================================================
/**
 * Register a consumer to get called on the equivalent of OnKeyDown.
 * This will call the consumer when all the modifiers are met and the character
 * has been received. If a modifier such as MOD_SHIFT is set but the key is not
 * pressed then the consumer will not be called.
 *
 * If there are duplicate listeners for the same exact event the previous one
 * will be removed.
 *
 * @param inConsumer the consumer to add for events, this will be deleted by this.
 * @param inModifiers mask of keyboard modifiers- MOD_SHIFT, MOD_CONTROL, MOD_MENU...
 * @param inVirtualKey the virtualkey the event is to occur on.
 */
//=============================================================================
void CHotKeys::RegisterKeyDownEvent(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                                    int inVirtualKey)
{
    // Remove any previous consumers.
    this->RemoveConsumers(inModifiers, inVirtualKey, &m_KeyDownMap);
    this->InsertConsumer(inConsumer, inModifiers, inVirtualKey, &m_KeyDownMap);
}

//=============================================================================
/**
 * Register a consumer to get called on the equivalent of OnKeyUp.
 * This will call the consumer when all the modifiers are met and the character
 * has been received. If a modifier such as MOD_SHIFT is set but the key is not
 * pressed then the consumer will not be called.
 *
 * If there are duplicate listeners for the same exact event the previous one
 * will be removed.
 *
 * @param inConsumer the consumer to add for events, this will be deleted by this.
 * @param inModifiers mask of keyboard modifiers- MOD_SHIFT, MOD_CONTROL, MOD_MENU...
 * @param inVirtualKey the virtualkey the event is to occur on.
 */
//=============================================================================
void CHotKeys::RegisterKeyUpEvent(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                                  int inVirtualKey)
{
    // Remove any previous consumers.
    this->RemoveConsumers(inModifiers, inVirtualKey, &m_KeyUpMap);
    this->InsertConsumer(inConsumer, inModifiers, inVirtualKey, &m_KeyUpMap);
}

//=============================================================================
/**
 * Register a consumer to get called on the equivalent of OnChar.
 * This will call the consumer when all the modifiers are met and the character
 * has been received. If a modifier such as MOD_SHIFT is set but the key is not
 * pressed then the consumer will not be called.
 *
 * If there are duplicate listeners for the same exact event the previous one
 * will be removed.
 *
 * @param inConsumer the consumer to which to add this key
 * @param inString the string containing key/mods.  This needs to be in the form of
 *key/:Mod/,Mod/,Mod ie x/:ctrl/,shift
 *				   see IDS_HK_SEPARATOR & IDS_HK_MOD_SEPARATOR & IDS_HK_CTRL -
 *IDS_HK_ALPHALOCK for useful info in creating these strings
 */
void CHotKeys::RegisterKeyEvent(CHotKeyConsumer *inConsumer, const Q3DStudio::CString &inString)
{
    Qt::KeyboardModifiers theMod;
    int theKey;
    GetModAndKeyFromString(inString, theMod, theKey);

    RegisterKeyEvent(inConsumer, theMod, theKey);
}

//=============================================================================
/**
 * Register a consumer to get called on the equivalent of OnKeyDown.
 * This will call the consumer when all the modifiers are met and the character
 * has been received. If a modifier such as MOD_SHIFT is set but the key is not
 * pressed then the consumer will not be called.
 *
 * If there are duplicate listeners for the same exact event the previous one
 * will be removed.
 *
 */
void CHotKeys::RegisterKeyDownEvent(CHotKeyConsumer *inConsumer, const Q3DStudio::CString &inString)
{
    Qt::KeyboardModifiers theMod;
    int theVirtualKey;
    GetModAndKeyFromString(inString, theMod, theVirtualKey);

    RegisterKeyDownEvent(inConsumer, theMod, theVirtualKey);
}

//=============================================================================
/**
 * Register a consumer to get called on the equivalent of OnKeyUp.
 * This will call the consumer when all the modifiers are met and the character
 * has been received. If a modifier such as MOD_SHIFT is set but the key is not
 * pressed then the consumer will not be called.
 *
 * If there are duplicate listeners for the same exact event the previous one
 * will be removed.
 *
 */
void CHotKeys::RegisterKeyUpEvent(CHotKeyConsumer *inConsumer, const Q3DStudio::CString &inString)
{
    Qt::KeyboardModifiers theMod;
    int theVirtualKey;
    GetModAndKeyFromString(inString, theMod, theVirtualKey);

    RegisterKeyUpEvent(inConsumer, theMod, theVirtualKey);
}

//=============================================================================
/**
 * Remove a previous key char event.
 * This removes a consumer from the list of events, the consumer is also deleted.
 *
 * @param inModifiers the modifiers for the event.
 * @param inCharacter the character for the event.
 */
//=============================================================================
void CHotKeys::UnregisterKeyEvent(Qt::KeyboardModifiers inModifiers, int inCharacter)
{
    this->RemoveConsumers(inModifiers, inCharacter, &m_KeyCharMap);
}

//=============================================================================
/**
 * Remove a previous key down event.
 * This removes a consumer from the list of events, the consumer is also deleted.
 *
 * @param inModifiers the modifiers for the event.
 * @param inVirtualKey the virtualkey for the event.
 */
//=============================================================================
void CHotKeys::UnregisterKeyDownEvent(Qt::KeyboardModifiers inModifiers, int inVirtualKey)
{
    this->RemoveConsumers(inModifiers, inVirtualKey, &m_KeyDownMap);
}

//=============================================================================
/**
 * Remove a previous key up event.
 * This removes a consumer from the list of events, the consumer is also deleted.
 *
 * @param inModifiers the modifiers for the event.
 * @param inVirtualKey the virtualkey for the event.
 */
//=============================================================================
void CHotKeys::UnregisterKeyUpEvent(Qt::KeyboardModifiers inModifiers, int inVirtualKey)
{
    this->RemoveConsumers(inModifiers, inVirtualKey, &m_KeyUpMap);
}

void CHotKeys::UnregisterKeyEvent(const Q3DStudio::CString &inString)
{
    Qt::KeyboardModifiers theMod;
    int theKey;
    GetModAndKeyFromString(inString, theMod, theKey);

    UnregisterKeyEvent(theMod, theKey);
}

void CHotKeys::UnregisterKeyDownEvent(const Q3DStudio::CString &inString)
{
    Qt::KeyboardModifiers theMod;
    int theVirtualKey;
    GetModAndKeyFromString(inString, theMod, theVirtualKey);

    UnregisterKeyDownEvent(theMod, theVirtualKey);
}

void CHotKeys::UnregisterKeyUpEvent(const Q3DStudio::CString &inString)
{
    Qt::KeyboardModifiers theMod;
    int theVirtualKey;
    GetModAndKeyFromString(inString, theMod, theVirtualKey);

    UnregisterKeyUpEvent(theMod, theVirtualKey);
}

//=============================================================================
/**
 * Called by owner window on a WM_CHAR event.
 *
 * @param inChar the character received.
 * @param inRepeatCount the repeat count.
 * @param inFlags the flags.
 */
//=============================================================================
bool CHotKeys::OnChar(int inChar, int inRepeatCount, Qt::KeyboardModifiers inFlags)
{
    return FireEvent(inChar, inRepeatCount, inFlags, &m_KeyCharMap);
}

//=============================================================================
/**
 * Called by owner window on a WM_KEYDOWN event.
 *
 * @param inChar the character received.
 * @param inRepeatCount the repeat count.
 * @param inFlags the flags.
 */
//=============================================================================
bool CHotKeys::OnKeyDown(int inVirtualKey, int inRepeatCount,
                         Qt::KeyboardModifiers modifiers)
{
    return FireEvent(inVirtualKey, inRepeatCount, modifiers, &m_KeyDownMap);
}

//=============================================================================
/**
 * Called by owner window on a WM_KEYUP event.
 *
 * @param inChar the character received.
 * @param inRepeatCount the repeat count.
 * @param inFlags the flags.
 */
//=============================================================================
bool CHotKeys::OnKeyUp(int inVirtualKey, int inRepeatCount, Qt::KeyboardModifiers modifiers)
{
    return FireEvent(inVirtualKey, inRepeatCount, modifiers, &m_KeyUpMap);
}

//=============================================================================
/**
 * Helper function for adding consumers to the map.
 *
 * @param inConsumer the consumer to be added.
 * @param inModifiers the keyboard modifiers.
 * @param inCharacter the character to be pressed.
 * @param inMap the map the consumer is to be added to.
 */
//=============================================================================
void CHotKeys::InsertConsumer(CHotKeyConsumer *inConsumer, Qt::KeyboardModifiers inModifiers,
                              int inCharacter, THotKeyMap *inMap)
{
    SHotKeyInfo *theInfo = new SHotKeyInfo;
    theInfo->Consumer = inConsumer;
    theInfo->Character = inCharacter;
    theInfo->Modifiers = inModifiers;

    THotKeyMap::iterator theMapPos = inMap->find(inCharacter);
    // There is already a list, just add this one to it.
    if (theMapPos != inMap->end()) {
        theMapPos->second.push_back(theInfo);
    } else {
        // Add a new list to the map
        THotKeyList theList;
        theList.push_back(theInfo);
        inMap->insert(std::pair<unsigned int, THotKeyList>(inCharacter, theList));
    }
}

//=============================================================================
/**
 * Remove all the consumers listening for the specified event.
 *
 * @param inModifiers the keyboard modifiers.
 * @param inCharacter the character to be pressed.
 * @param inMap the map to remove the consumers from.
 */
//=============================================================================
void CHotKeys::RemoveConsumers(Qt::KeyboardModifiers inModifiers, int inCharacter,
                               THotKeyMap *inMap)
{
    THotKeyMap::iterator theMapPos = inMap->find(inCharacter);
    if (theMapPos != inMap->end()) {
        THotKeyList::iterator theListPos = theMapPos->second.begin();
        while (theListPos != theMapPos->second.end()) {
            if ((*theListPos)->Modifiers == inModifiers) {
                SHotKeyInfo *theInfo = (*theListPos);
                delete theInfo->Consumer;
                delete theInfo;

                theListPos = theMapPos->second.erase(theListPos);
            } else
                ++theListPos;
        }
    }
}

//=============================================================================
/**
 * Fire off an event for the specified char.
 * This will go through the map for the event and find all applicable consumers
 * then call OnHotKeyEvent on that consumer.
 *
 * @param inChar the character that was pressed.
 * @param inRepeatCount the number of times the character was pressed.
 * @param inFlags the flags from the WM_KEY* call.
 * @param inMap the map to find the consumers in.
 * @return true if the event was dispatched.
 */
//=============================================================================
bool CHotKeys::FireEvent(int inChar, int inRepeatCount, Qt::KeyboardModifiers modifiers,
                         THotKeyMap *inMap)
{
    bool theRetVal = false;

    // Get the state of the keys before iterating through the key events. If a key event takes a
    // while to
    // be dispatched then the state of the modifier key may change and will cause incorrect results.
    unsigned int isMenuDown = modifiers & Qt::AltModifier;
    unsigned int isControlDown = modifiers & Qt::ControlModifier;
    unsigned int isShiftDown = modifiers & Qt::ShiftModifier;

    // If the keys are captured by a window and not modified, then don't send this
    // event to the registered consumers. This is mainly a hack to allow the command
    // window to work properly.
    if (m_CaptureKeys && !(isMenuDown || isControlDown || isShiftDown))
        return false;

    // Find the list of consumers for inChar
    THotKeyMap::iterator theMapPos = inMap->find(inChar);
    if (theMapPos != inMap->end()) {
        // Go through all the consumers checking if their key modifiers are met
        THotKeyList::iterator theListPos = theMapPos->second.begin();
        for (; theListPos != theMapPos->second.end(); ++theListPos) {
            SHotKeyInfo *theHotKeyInfo = *theListPos;
            // Check to see if the modifier is set and the key is down or neither of them are set
            // for each of the modifiers.
            if ((isMenuDown == (theHotKeyInfo->Modifiers & Qt::AltModifier))
                && (isControlDown == (theHotKeyInfo->Modifiers & Qt::ControlModifier))
                && (isShiftDown == (theHotKeyInfo->Modifiers & Qt::ShiftModifier))) {
                // If all the modifiers are correct then call the event.
                theRetVal = theHotKeyInfo->Consumer->OnHotKeyEvent(inChar, inRepeatCount, modifiers);
            }
        }
    }

    return theRetVal;
}

//=============================================================================
/**
 * Indicates whether the specified key is currently down or not.
 * @param inKeyCode The key to be checked
 * @return true if the key is down, otherwise false
 */
bool CHotKeys::IsKeyDown(Qt::KeyboardModifier inKeyCode)
{
    return qApp->keyboardModifiers() & inKeyCode;
}

//=============================================================================
/**
 * Retrieves the current status of the various modifier keys.
 * @return flag containing all of the modifier keys that are currently depressed
 */
Qt::KeyboardModifiers CHotKeys::GetCurrentKeyModifiers()
{
    return qApp->keyboardModifiers();
}

bool CHotKeys::isFocusOnControlThatWantsKeys()
{
    auto widgetControl = dynamic_cast<WidgetControl *>(qApp->focusObject());
    if (widgetControl) {
        auto control = widgetControl->getControl();
        if (control) {
            auto te = dynamic_cast<CTextEdit *>(control->FocusedChild());
            if (te && !te->IsReadOnly())
                return true;
        }
    } else {
        auto quickWidget = dynamic_cast<QQuickWidget *>(qApp->focusObject());
        if (quickWidget) {
            auto focusItem = quickWidget->quickWindow()->activeFocusItem();
            if (focusItem && focusItem->property("ignoreHotkeys").toBool())
                return true;
        }
    }
    return false;
}

//=============================================================================
/**
 * returns the mod and key.... mod will include all mods that came in comma delimeted
 *
 * @param inString a string containing a key combo to parse
 */
void CHotKeys::GetModAndKeyFromString(const Q3DStudio::CString &inString, Qt::KeyboardModifiers &inMod,
                                      int &inKey)
{
    inMod = 0;
    inKey = 0;
    CStringTokenizer theTokenizer(inString, L"/:"); // TODO: Localize
    Q3DStudio::CString theMods = theTokenizer.GetCurrentPartition();
    ++theTokenizer;
    Q3DStudio::CString theKey = theTokenizer.GetCurrentPartition();
    CStringTokenizer theModTokenizer(theMods, L"/,"); // TODO: Localize
    while (theModTokenizer.HasNextPartition()) {
        Q3DStudio::CString theCurrentMod = theModTokenizer.GetCurrentPartition();
        inMod |= GetModifierFromString(theCurrentMod);
        ++theModTokenizer;
    }
    inKey = GetKeyFromString(theKey);
}

//=============================================================================
/**
 * returns the unsigned int equivalent of a string modifer type
 *
 * @param inString a string containing a modifier
 */
Qt::KeyboardModifier CHotKeys::GetModifierFromString(const Q3DStudio::CString &inString)
{
    Qt::KeyboardModifier theRetVal = Qt::NoModifier;

    if (inString == L"ctrl") // TODO: Localize
        theRetVal = Qt::ControlModifier;
    else if (inString == L"shift") // TODO: Localize
        theRetVal = Qt::ShiftModifier;
    else if (inString == L"alt") // TODO: Localize
        theRetVal = Qt::AltModifier;
    return theRetVal;
}

//=============================================================================
/**
 * Finds a character or a unicode key depending on  lenght of the string
 *
 * @param inChar the ascii or key code for the char
 */
int CHotKeys::GetKeyFromString(Q3DStudio::CString inChar)
{
    int theRetVal = 0;
    if (inChar.Length() == 1) {
        Q3DStudio::CString theLowerCase = inChar.ToLower();
        char theChar = static_cast<char>(theLowerCase[(long)0]);
        theRetVal = theChar;
    } else if (inChar.Length() > 1) {
        theRetVal = atoi(inChar.GetCharStar());
    }
    return theRetVal;
}

void CHotKeys::CaptureHotKeys()
{
    m_CaptureKeys = true;
}

void CHotKeys::ReleaseHotKeys()
{
    m_CaptureKeys = false;
}
