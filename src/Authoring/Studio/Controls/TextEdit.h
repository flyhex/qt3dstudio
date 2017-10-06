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

#ifndef INCLUDED_TEXT_EDIT_H
#define INCLUDED_TEXT_EDIT_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "Control.h"
#include "HotKeys.h"
#include "StudioPreferences.h"
#include "ITickTock.h"
#include "Multicaster.h"

//==============================================================================
//	Forwards
//==============================================================================

class CRenderer;

//==============================================================================
/**
 * @class CTextEdit Base class for textual edit fields in Studio (timeline, inspector).
 */

// Functors
GENERIC_FUNCTOR_1(CCommitDataListener, OnSetData, CControl *);
GENERIC_FUNCTOR_1(ITextEditEnterListener, OnEnter, CControl *);
GENERIC_FUNCTOR_2(IChangeDataListener, OnChangeData, const Q3DStudio::CString &,
                  const Q3DStudio::CString &);

class CTextEdit : public CControl
{
    struct SCaret
    {
        unsigned long position; ///< index of the character that the cursor is currently in front of
                                ///(zero is first character)
        ::CColor color; ///< color of the cursor
        bool show; ///< must be true in order for the cursor to show up (outside of blink state)
        bool visible; ///< blinking state of the cursor (true if currently visible)
    };

    typedef std::vector<float> TCharLengths;

public:
    enum EAlignment { LEFT = 0, CENTER, RIGHT };

    CTextEdit();
    virtual ~CTextEdit();

    DEFINE_OBJECT_COUNTER(CTextEdit)

    virtual Q3DStudio::CString
    GetString() = 0; ///< Returns the string to be displayed in the edit box

    void Draw(CRenderer *inRenderer) override;
    void OnMouseOver(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inLocation, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnChar(const QString &inChar, Qt::KeyboardModifiers inModifiers) override;
    bool OnKeyDown(unsigned int inChar, Qt::KeyboardModifiers inModifiers) override;
    void OnGainFocus() override;
    void OnLoseFocus() override;
    bool CanGainFocus() override;
    virtual bool CanAcceptChar(const Q3DStudio::CString &inCheckString, unsigned int inChar,
                               unsigned int inPosition);
    void SetAlignment(EAlignment inAlignment);
    void SetBufferLength(long inLength);
    void SetMaxLength(long inLength);
    void SetFillBackground(bool inFillBackground);
    void SelectAllText();
    void DisplayContextMenu(CPt inPt);
    virtual bool CanPaste() = 0;

    virtual void SetData(const Q3DStudio::CString &inData, bool inFireEvent = true) = 0;
    virtual void ReloadData();
    virtual void SetPrefix(const Q3DStudio::CString &inPrefix);
    virtual void SetPostfix(const Q3DStudio::CString &inPostfix);

    void SetReadOnly(bool inReadOnly);
    bool IsReadOnly();
    void SetTextColor(::CColor inColor);
    ::CColor GetTextColor();
    void SetUseBGColorReadOnly(bool inUseBGColorReadOnly);
    ::CColor SetBGColorNoFocus(::CColor inColor);
    ::CColor SetBGColorFocus(::CColor inColor);
    void SetBoldText(bool inBold);

    void AddCommitListener(CCommitDataListener *inListener);
    void RemoveCommitListener(CCommitDataListener *inListener);
    void FireCommitEvent();

    void AddChangeListener(IChangeDataListener *inListener);
    void RemoveChangeListener(IChangeDataListener *inListener);
    void FireChangeEvent(const Q3DStudio::CString &inOldString,
                         const Q3DStudio::CString &inNewString);

    void AddEnterListener(ITextEditEnterListener *inListener);
    void RemoveEnterListener(ITextEditEnterListener *inListener);
    void FireEnterEvent();

    bool HasSelectedText();
    long GetSelectionLeft();
    long GetSelectionRight();
    void ClearSelection();
    virtual void SetSelection(long inFirstIndex, long inLastIndex);

    void MoveCaretTo(long inPosition, bool inSelect = false, bool inScroll = true);
    void Scroll(long inPixel);
    virtual void EnterText(bool inHighlight);

    virtual void RefreshDisplayFromData() = 0;

    bool CopyText();
    bool PasteText();
    bool CutText();
    bool IsDirty() const;

    virtual void OnTimer();

    // Public so that derived objects can query the value on change events
    Q3DStudio::CString GetDisplayString();

protected:
    virtual void DoFillBackground(CRenderer *inRenderer);

protected:
    bool m_IsStringDirty;
    bool m_NeedsCommit;
    TCharLengths m_CharWidths;
    EAlignment m_Alignment;
    float m_TotalCharWidth;
    SCaret m_Caret;
    long m_BufferLength;
    long m_MaxLength;
    bool m_IsReadOnly;
    bool m_MouseIsDown;
    CPt m_StartDragPt;
    CHotKeys *m_CommandHandler;
    bool m_FillBackground;
    ::CColor m_TextColor;
    bool m_UseBGColorReadOnly; ///< Flag indicating that we should use the background colors in read
                               ///only boxes as well as read/write
    ::CColor m_BackgroundColorNoFocus;
    ::CColor m_BackgroundColorFocus;
    Q3DStudio::CString m_DisplayString;
    Q3DStudio::CString m_Prefix;
    Q3DStudio::CString m_Postfix;
    bool m_BoldText;

    virtual void DeleteCurrentSelection(bool inFireEvent = true);
    virtual void DeleteCharacter(bool inIsOnRight = false);
    void RegisterCommands();
    virtual void SetDirty(bool inDirty);
    float CalculateCharWidths(CRenderer *inRenderer);
    virtual bool HandleSpecialChar(unsigned int inChar, Qt::KeyboardModifiers inFlags);
    void DrawText(CRenderer *inRenderer);
    void DrawCaret(CRenderer *inRenderer);
    CRct GetTextRect();
    long ConvertIndexToPosition(long inIndex);
    long ConvertPositionToIndex(CPt inPosition);
    void SetDisplayString(const Q3DStudio::CString &inString, bool inFireEvent = true);
    long GetBufferLength();
    long GetMaxLength();
    void ResetBlinkingCursor();
    bool InsertChar(unsigned int inChar);

private:
    CMulticaster<CCommitDataListener *> m_CommitListeners;
    CMulticaster<IChangeDataListener *> m_ChangeListeners;
    CMulticaster<ITextEditEnterListener *> m_EnterListeners;
    long m_SelectionStart;
    long m_SelectionEnd;
    CPt m_ScrollAmount; ///< Pixel offset to scroll the text and caret by
    std::shared_ptr<UICDM::ISignalConnection> m_TimerConnection;
};

#endif // INCLUDED_TEXT_EDIT_H
