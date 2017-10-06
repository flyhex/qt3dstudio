/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#ifndef PATH_ANCHOR_POINT_TIMELINE_ITEM_BINDING
#define PATH_ANCHOR_POINT_TIMELINE_ITEM_BINDING
#pragma once

#include "UICDMTimelineItemBinding.h"
#include "UICDMDataTypes.h"
#include <boost/tuple/tuple.hpp>

//==============================================================================
//	Classes
//==============================================================================
class CTimelineTranslationManager;
class CBaseStateRow;

//=============================================================================
/**
 * Binding to a UICDM object of Material type
 */
class CPathAnchorPointTimelineItemBinding : public CUICDMTimelineItemBinding
{
public: // Construction
    CPathAnchorPointTimelineItemBinding(CTimelineTranslationManager *inMgr,
                                        UICDM::CUICDMInstanceHandle inDataHandle);

    bool ShowToggleControls() const override { return false; }
    bool IsVisible() const override { return true; }
    void SetVisible(bool) override {}
    ITimelineTimebar *GetTimebar() override;
    bool IsLocked() const override { return false; }
    void SetLocked(bool) override {}
    bool IsShy() const override { return false; }
    void SetShy(bool) override {}
    Q3DStudio::CString GetName() const override { return L"Anchor Point"; }
    void SetName(const Q3DStudio::CString &) override {}
};

#endif
