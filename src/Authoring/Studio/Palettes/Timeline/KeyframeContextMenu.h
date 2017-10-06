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

#ifndef INCLUDED_KEYFRAME_CONTEXT_MENU
#define INCLUDED_KEYFRAME_CONTEXT_MENU 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include <QMenu>

class ITimelineKeyframesManager;
class ITimelineItemProperty;
class ITimelineTimebar;
class ITimeChangeCallback;
class ITimelineItemKeyframesHolder;
class ITimebarControl;

class QAction;

//=============================================================================
/**
 * Abstract class that contain the common items for the keyframes-related context menu.
 */
class CBaseKeyframeContextMenu : public QMenu
{
    Q_OBJECT
public:
    CBaseKeyframeContextMenu(QWidget *parent = nullptr);
    virtual ~CBaseKeyframeContextMenu();

protected:
    virtual void Initialize(ITimelineKeyframesManager *inKeyframesManager);
    virtual void MakeDynamic();
    virtual ITimelineItemKeyframesHolder *GetKeyframesHolder() { return nullptr; }

protected Q_SLOTS:
    void CutSelectedKeys();
    void CopySelectedKeys();
    void PasteSelectedKeys();
    void DeleteSelectedKeys();
    void InsertKeyframe();
    void DeleteChannelKeys();

protected:
    bool m_HasDynamicSelectedKeyframes;
    ITimelineKeyframesManager *m_KeyframesManager;
    QAction *m_insertAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_deleteSelectedAction;
    QAction *m_deleteChannelKeysAction;
};

//=============================================================================
/**
 * Context menu right-clicking on a timebar,
 */
class CTimebarKeyframeContextMenu : public CBaseKeyframeContextMenu
{
    Q_OBJECT
public:
    CTimebarKeyframeContextMenu(ITimebarControl *inTimebarControl,
                                ITimelineKeyframesManager *inKeyframesManager,
                                bool inShowTimebarPropertiesOptions = true,
                                QWidget *parent = nullptr);
    ~CTimebarKeyframeContextMenu();

protected:
    void Initialize(ITimelineKeyframesManager *inKeyframesManager) override;
    ITimelineItemKeyframesHolder *GetKeyframesHolder() override;
    void MakeDynamic() override;

    void ChangeTimebarColor();
    void ChangeTimebarText();
    void ToggleTimebarHandles();
    void SetTimebarTime();

protected:
    ITimebarControl *m_TimebarControl;
    bool m_ShowTimebarPropertiesOptions;

    QAction *m_makeDynamicAction;
    QAction *m_timeBarColorAction;
    QAction *m_timeBarTextAction;
    QAction *m_timeBarHandlesAction;
    QAction *m_timeBarTimeAction;
};

//=============================================================================
/**
 * Context menu for right-clicking on selected keyframe(s)
 * so even though the assoicated action isn't limited to the keyframe that was right-clicked on,
 * this is still a different one from right-clicking on outside the keyframe.
 * This is how the old system used to be.
 */
class CKeyframeContextMenu : public CBaseKeyframeContextMenu
{
    Q_OBJECT
public:
    CKeyframeContextMenu(ITimelineKeyframesManager *inKeyframesManager,
                         ITimelineItemProperty *inTimelineItemProperty = nullptr,
                         QWidget *parent = nullptr);
    ~CKeyframeContextMenu();

    void SetTime(long inTime);

protected:
    void Initialize(ITimelineKeyframesManager *inKeyframesManager) override;
    void MakeDynamic() override;

protected Q_SLOTS:
    void SetInterpolation();
    void SetKeyframeTime();

protected:
    long m_Time;

private:
    ITimelineItemProperty *m_TimelineItemProperty;
    QAction *m_makeDynamicAction;
    QAction *m_setKeyframeTimeAction;
    QAction *m_setInterpolationAction;
};
#endif // INCLUDED_KEYFRAME_CONTEXT_MENU
