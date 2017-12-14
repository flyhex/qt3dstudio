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

#include "stdafx.h"
#include "Strings.h"

//==============================================================================
//	Includes
//==============================================================================
#include "KeyframeContextMenu.h"
#include "IDoc.h"
#include "CColor.h"
#include "Preferences.h"
#include "Dialogs.h"
#include "TimebarControl.h"
#include "Bindings/ITimelineTimebar.h"
#include "Bindings/ITimelineKeyframesManager.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineItemProperty.h"
#include "IKeyframe.h"

#include <QColorDialog>

#define IDC_KEYFRAME_COLOR_BOX 1002

//=============================================================================
/**
 * Constructor
 */
CBaseKeyframeContextMenu::CBaseKeyframeContextMenu(QWidget *parent)
    : QMenu(parent)
    , m_HasDynamicSelectedKeyframes(false)
    , m_KeyframesManager(nullptr)
{
}

//=============================================================================
/**
 * Destructor
 */
CBaseKeyframeContextMenu::~CBaseKeyframeContextMenu()
{
}

void CBaseKeyframeContextMenu::Initialize(ITimelineKeyframesManager *inKeyframesManager)
{
    m_KeyframesManager = inKeyframesManager;
    ASSERT(m_KeyframesManager);

    //"Insert Keyframe"
    ITimelineItemKeyframesHolder *theKeyframesHolder = GetKeyframesHolder();
    if (theKeyframesHolder) {
        m_insertAction = new QAction(tr("Insert Keyframe"));
        connect(m_insertAction, &QAction::triggered, this, &CBaseKeyframeContextMenu::InsertKeyframe);
        addAction(m_insertAction);
    }

    bool theHasKeysSelected = m_KeyframesManager->HasSelectedKeyframes();
    bool theCanCopyKeys = m_KeyframesManager->CanPerformKeyframeCopy();
    bool theCanPasteKeys = m_KeyframesManager->CanPerformKeyframePaste();

    m_cutAction = new QAction(tr("Cut Selected Keyframes"));
    connect(m_cutAction, &QAction::triggered, this, &CBaseKeyframeContextMenu::CutSelectedKeys);
    m_cutAction->setEnabled(theCanCopyKeys);
    addAction(m_cutAction);

    m_copyAction = new QAction(tr("Copy Selected Keyframes"));
    connect(m_copyAction, &QAction::triggered, this, &CBaseKeyframeContextMenu::CopySelectedKeys);
    m_copyAction->setEnabled(theCanCopyKeys);
    addAction(m_copyAction);

    m_pasteAction = new QAction(tr("Paste Keyframes"));
    connect(m_pasteAction, &QAction::triggered, this, &CBaseKeyframeContextMenu::PasteSelectedKeys);
    m_pasteAction->setEnabled(theCanPasteKeys);
    addAction(m_pasteAction);

    m_deleteSelectedAction = new QAction(tr("Delete Selected Keyframes"));
    connect(m_deleteSelectedAction, &QAction::triggered, this, &CBaseKeyframeContextMenu::DeleteSelectedKeys);
    m_deleteSelectedAction->setEnabled(theHasKeysSelected);
    addAction(m_deleteSelectedAction);

    //"Delete All Channel Keyframes"
    if (theKeyframesHolder) {
        m_deleteChannelKeysAction = new QAction(tr("Delete All Channel Keyframes"));
        connect(m_deleteChannelKeysAction, &QAction::triggered, this, &CBaseKeyframeContextMenu::DeleteChannelKeys);
        addAction(m_deleteChannelKeysAction);
    }
}

//=============================================================================
/**
 * Called when the cut selected keys option is chosen by the user.  Makes a call
 * to the doc to handle the request
 */
void CBaseKeyframeContextMenu::InsertKeyframe()
{
    GetKeyframesHolder()->InsertKeyframe();
}

//=============================================================================
/**
 * Called when the cut selected keys option is chosen by the user.  Makes a call
 * to the doc to handle the request
 */
void CBaseKeyframeContextMenu::DeleteChannelKeys()
{
    GetKeyframesHolder()->DeleteAllChannelKeyframes();
}

//=============================================================================
/**
 * Called when the cut selected keys option is chosen by the user.  Makes a call
 * to the doc to handle the request
 */
void CBaseKeyframeContextMenu::CutSelectedKeys()
{
    m_KeyframesManager->RemoveKeyframes(true);
}

//=============================================================================
/**
 * Called when the copy selected keys option is chosen by the user.  Makes a call
 * to the doc to handle the request
 */
void CBaseKeyframeContextMenu::CopySelectedKeys()
{
    m_KeyframesManager->CopyKeyframes();
}

//=============================================================================
/**
 * Called when the paste selected keys option is chosen by the user.  Makes a call
 * to the doc to handle the request
 */
void CBaseKeyframeContextMenu::PasteSelectedKeys()
{
    m_KeyframesManager->PasteKeyframes();
}

//=============================================================================
/**
 * Called when the delete selected keys option is chosen by the user.  Makes a call
 * to the doc to handle the request
 */
void CBaseKeyframeContextMenu::DeleteSelectedKeys()
{
    m_KeyframesManager->RemoveKeyframes(false);
}

//=============================================================================
/**
 * Toggle if the selected keyframe(s) is dynamic
 */
void CBaseKeyframeContextMenu::MakeDynamic()
{
    m_KeyframesManager->SetKeyframesDynamic(!m_HasDynamicSelectedKeyframes); // toggle
}

//==============================================================================
//	CTimebarKeyframeContextMenu
//==============================================================================
CTimebarKeyframeContextMenu::CTimebarKeyframeContextMenu(ITimebarControl *inTimebarControl, ITimelineKeyframesManager *inKeyframesManager,
    bool inShowTimebarPropertiesOptions /*= true */, QWidget *parent)
    : CBaseKeyframeContextMenu(parent)
    , m_TimebarControl(inTimebarControl)
    , m_ShowTimebarPropertiesOptions(inShowTimebarPropertiesOptions)
{
    Initialize(inKeyframesManager);
}

CTimebarKeyframeContextMenu::~CTimebarKeyframeContextMenu()
{
}

void CTimebarKeyframeContextMenu::Initialize(ITimelineKeyframesManager *inKeyframesManager)
{
    CBaseKeyframeContextMenu::Initialize(inKeyframesManager);

    // Dynamic keyframes ( the way it was set up before was, this option is still shown, but grayed
    // out even if this context menu isn't triggered from right-clicking on a timebar )
    QString theMakeDynamicOption = tr("Make Animations Static");

    if (m_TimebarControl->GetKeyframesHolder()->GetKeyframeCount() > 0) {
        m_HasDynamicSelectedKeyframes = m_KeyframesManager->HasDynamicKeyframes();

        if (!m_HasDynamicSelectedKeyframes) {
            theMakeDynamicOption = tr("Make Animations Dynamic");
        }

        m_makeDynamicAction = new QAction(theMakeDynamicOption);
        connect(m_makeDynamicAction, &QAction::triggered, this, &CTimebarKeyframeContextMenu::MakeDynamic);
        addAction(m_makeDynamicAction);
    }

    if (m_ShowTimebarPropertiesOptions) {
        // Timebar specific actions
        addSeparator();
        m_timeBarColorAction = new QAction(tr("Change Time Bar Color"));
        connect(m_timeBarColorAction, &QAction::triggered, this, &CTimebarKeyframeContextMenu::ChangeTimebarColor);

        m_timeBarTextAction = new QAction(tr("Change Time Bar Text"));
        connect(m_timeBarTextAction, &QAction::triggered, this, &CTimebarKeyframeContextMenu::ChangeTimebarText);

        // Change the text for the timebar option depending on whether they are currently being
        // shown or not
        QString theTimebarHandleTextID = tr("Show Time Bar Handles");
        if (CPreferences::GetUserPreferences("Timeline").GetValue("ShowTimebarHandles", false))
            theTimebarHandleTextID = tr("Hide Time Bar Handles");

        m_timeBarHandlesAction = new QAction(theTimebarHandleTextID);
        connect(m_timeBarHandlesAction, &QAction::triggered, this, &CTimebarKeyframeContextMenu::ToggleTimebarHandles);

        m_timeBarTimeAction = new QAction(tr("Set Timebar Time"));
        connect(m_timeBarTimeAction, &QAction::triggered, this, &CTimebarKeyframeContextMenu::SetTimebarTime);
    }
}

void CTimebarKeyframeContextMenu::MakeDynamic()
{
    m_KeyframesManager->SelectAllKeyframes();
    CBaseKeyframeContextMenu::MakeDynamic();
}

//=============================================================================
/**
 * To show "Insert Keyframe" and "Delete All Channel Keyframes"
 */
ITimelineItemKeyframesHolder *CTimebarKeyframeContextMenu::GetKeyframesHolder()
{
    return m_TimebarControl->GetKeyframesHolder();
}

//=============================================================================
/**
 * Called when the copy selected keys option is chosen by the user.
 */
void CTimebarKeyframeContextMenu::ChangeTimebarColor()
{
    QColor previousColor = m_TimebarControl->GetTimebarColor();
    QColorDialog *theColorDlg = new QColorDialog(previousColor, this);
    theColorDlg->setOption(QColorDialog::DontUseNativeDialog, true);
    connect(theColorDlg, &QColorDialog::currentColorChanged,
            this, &CTimebarKeyframeContextMenu::onTimeBarColorChanged);
    if (theColorDlg->exec() == QDialog::Accepted)
        m_TimebarControl->SetTimebarColor(theColorDlg->selectedColor());
    else
        m_TimebarControl->SetTimebarColor(previousColor);
}

void CTimebarKeyframeContextMenu::onTimeBarColorChanged(const QColor &color)
{
    m_TimebarControl->SetTimebarColor(color);
}

//=============================================================================
/**
 * Called when the copy selected keys option is chosen by the user.
 */
void CTimebarKeyframeContextMenu::ChangeTimebarText()
{
    m_TimebarControl->OnEditTimeComment();
}

//=============================================================================
/**
 * Shows or hides timebar handles in the timeline.  If timebar handles are
 * currently being shown, they are hidden, and the preference is stored.  If
 * timebar handles are being hidden, they are shown.
 */
void CTimebarKeyframeContextMenu::ToggleTimebarHandles()
{
    // Get the current timebar handle preference
    bool theHandlesAreShowing =
        CPreferences::GetUserPreferences("Timeline").GetValue("ShowTimebarHandles", false);
    // Switch the preference.
    CPreferences::GetUserPreferences("Timeline")
        .SetValue("ShowTimebarHandles", !theHandlesAreShowing);
    if (m_TimebarControl)
        m_TimebarControl->OnToggleTimebarHandles();
}

//=============================================================================
/**
  * SetTimebarTime:  This is the event handler that will be called when the user
  *					chooses set timebar time from a pop up menu. This pop up
  *menu
  *					is triggered when the user right click on the selected
  *timebar.
  *					It displays a time edit dialog to allow the user to set the
  *					start and end time of the timebar time.
  * @param NONE
  * @return NONE
  */
void CTimebarKeyframeContextMenu::SetTimebarTime()
{
    m_TimebarControl->SetTimebarTime();
}

//==============================================================================
//	CKeyframeContextMenu
//==============================================================================
CKeyframeContextMenu::CKeyframeContextMenu(ITimelineKeyframesManager *inKeyframesManager,
                                           ITimelineItemProperty *inTimelineItemProperty, QWidget *parent)
    : CBaseKeyframeContextMenu(parent)
    , m_TimelineItemProperty(inTimelineItemProperty)
{
    Initialize(inKeyframesManager);
}

CKeyframeContextMenu::~CKeyframeContextMenu()
{
}

void CKeyframeContextMenu::Initialize(ITimelineKeyframesManager *inKeyframesManager)
{
    CBaseKeyframeContextMenu::Initialize(inKeyframesManager);

    // Dynamic keyframes ( the way it was set up before was, this option is still shown, but grayed
    // out even if this context menu isn't triggered from right-clicking on a timebar )
    QString theMakeDynamicOption = tr("Make Animations Static");
    m_HasDynamicSelectedKeyframes =
            m_TimelineItemProperty && m_TimelineItemProperty->IsDynamicAnimation();

    if (!m_HasDynamicSelectedKeyframes) {
        theMakeDynamicOption = tr("Make Animations Dynamic");
    }

    m_makeDynamicAction = new QAction(theMakeDynamicOption);
    connect(m_makeDynamicAction, &QAction::triggered, this, &CKeyframeContextMenu::MakeDynamic);
    addAction(m_makeDynamicAction);

    addSeparator();
    bool theHasKeysSelected = m_KeyframesManager->HasSelectedKeyframes();
    m_setInterpolationAction = new QAction(tr("Set Interpolation"));
    connect(m_setInterpolationAction, &QAction::triggered, this, &CKeyframeContextMenu::SetInterpolation);
    addAction(m_setInterpolationAction);

    m_setKeyframeTimeAction = new QAction(tr("Set Keyframe Time"));
    connect(m_setKeyframeTimeAction, &QAction::triggered, this, &CKeyframeContextMenu::SetKeyframeTime);
    m_setKeyframeTimeAction->setEnabled(theHasKeysSelected);
    addAction(m_setKeyframeTimeAction);
}

void CKeyframeContextMenu::MakeDynamic()
{
    if (m_TimelineItemProperty != nullptr && m_TimelineItemProperty->GetKeyframeCount() > 0) {
        m_TimelineItemProperty->SelectKeyframes(
            true, m_TimelineItemProperty->GetKeyframeByIndex(0)->GetTime());
    }

    CBaseKeyframeContextMenu::MakeDynamic();
}

//=============================================================================
/**
  * SetTime: Saves the keyframe time
  * @param inTime is the keyframe time
  */
void CKeyframeContextMenu::SetTime(long inTime)
{
    m_Time = inTime;
}

//=============================================================================
/**
 * Called when the set interpolation option is taken by the user.  Uses the left most
 * selected key for the base interpolation in the dialog box.  User can then set the interpolation
 * and it is propagated to all teh keys
 */
void CKeyframeContextMenu::SetInterpolation()
{
    m_KeyframesManager->SetKeyframeInterpolation();
}

//=============================================================================
/**
  * SetKeyframeTime: This is the event handler that will be called when the user
  *					chooses set keyframe time from a pop up menu. This pop up
  *menu
  *					is triggered when the user right click on the selected
  *keyframe.
  *					It displays a time edit dialog to allow the user to set the
  *					keyframe time.
  * @param NONE
  * @return NONE
  */
void CKeyframeContextMenu::SetKeyframeTime()
{
    m_KeyframesManager->SetKeyframeTime(m_Time);
}
