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

#ifndef INCLUDED_COMMENT_EDIT
#define INCLUDED_COMMENT_EDIT 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "TextEditInPlace.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;
class ITimelineTimebar;

//=============================================================================
/**
 * @class CCommentEdit this class handles changing the comments for a timebar in
 * the timeline.  This class seems necessary only to change the color of the text
 * depending on the timebar color
 */
class CCommentEdit : public CTextEditInPlace, public CCommitDataListener
{
public:
    CCommentEdit(ITimelineTimebar *inTimelineItemTimebar);
    virtual ~CCommentEdit();
    void OnSetData(CControl *inControl) override;
    bool OnMouseDoubleClick(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void Draw(CRenderer *inRenderer) override;
    void RefreshMetaData();
    void CalculateTextColor();
    bool HitTest(const CPt &inPoint) const override;
    void SetSelected(bool inState);

    void OnLoseFocus() override;
    void OnGainFocus() override;

    void DoChangeComment();

protected:
    ITimelineTimebar *m_TimelineItemTimebar;
    ::CColor m_Color;
    bool m_IsSelected;
};

#endif // INCLUDED_COMMENT_EDIT
