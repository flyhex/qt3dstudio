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

#ifndef INCLUDED_TIME_TOOLBAR_H
#define INCLUDED_TIME_TOOLBAR_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "TimeEdit.h"
#include "Control.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;
class IDoc;

//=============================================================================
/**
 * Control at the top of the time display and a header for the toggle column.
 */
class CTimeToolbar : public CControl, public CTimeEditChangeListener
{
public:
    CTimeToolbar(IDoc *inDoc);
    virtual ~CTimeToolbar();
    void Draw(CRenderer *inRenderer) override;

    void SetSize(CPt inSize) override;
    void SetTime(long inTime);
    virtual long GetTime();

    void OnTimeChanged(long inNewTime) override;

protected:
    CTimeEdit *m_TimeEdit;
    IDoc *m_Doc;
    CColor m_Color;
};

#endif // INCLUDED_TIME_TOOLBAR_H
