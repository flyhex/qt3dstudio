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

#ifndef INCLUDED_TOGGLE_TOOLBAR_H
#define INCLUDED_TOGGLE_TOOLBAR_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================

#include "FlowLayout.h"
#include "ProceduralButton.h"
#include "ToggleButton.h"

//==============================================================================
//	Forwards
//==============================================================================

class CRenderer;
class CTimelineTreeLayout;

//=============================================================================
/**
 * Control at the top of the time display and a header for the toggle column.
 */
class CToggleToolbar : public CFlowLayout
{
public:
    CToggleToolbar(CTimelineTreeLayout *inTreeLayout);
    virtual ~CToggleToolbar();
    void Draw(CRenderer *inRenderer) override;

    void FilterShy(bool inFilter);
    void FilterVisible(bool inFilter);
    void FilterLocked(bool inFilter);
    void OnButtonToggled(CToggleButton *inButton, CToggleButton::EButtonState inState);

protected:
    CProceduralButton<CToggleButton> *m_FltrShyBtn;
    CProceduralButton<CToggleButton> *m_FltrVisibleBtn;
    CProceduralButton<CToggleButton> *m_FltrLockBtn;
    CTimelineTreeLayout *m_TreeLayout;
    CColor m_Color;
};

#endif // INCLUDED_TOGGLE_TOOLBAR_H
