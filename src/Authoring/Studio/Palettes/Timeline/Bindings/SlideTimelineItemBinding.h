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
#ifndef INCLUDED_SLIDE_TIMELINEITEM_BINDING_H
#define INCLUDED_SLIDE_TIMELINEITEM_BINDING_H 1

#pragma once

#include "Qt3DSDMTimelineItemBinding.h"

//==============================================================================
//	Classes
//==============================================================================
class ITimelineItem;
class CTimelineTranslationManager;

//=============================================================================
/**
 * Binding to a DataModel object of Slide type
 */
class CSlideTimelineItemBinding : public Qt3DSDMTimelineItemBinding
{
public:
    CSlideTimelineItemBinding(CTimelineTranslationManager *inMgr,
                              qt3dsdm::Qt3DSDMInstanceHandle inDataHandle);
    ~CSlideTimelineItemBinding() {}

    // Qt3DSDMTimelineItemBinding
    ITimelineTimebar *GetTimebar() override;
    void SetName(const Q3DStudio::CString &inName) override;
    bool IsValidTransaction(EUserTransaction inTransaction) override;

    // No properties
    long GetPropertyCount() override { return 0; }
    ITimelineItemProperty *GetProperty(long) override { return nullptr; }

    // Eye/Lock toggles are not applicable
    bool ShowToggleControls() const override { return false; }
    bool IsLockedEnabled() const override { return false; }
    bool IsVisibleEnabled() const override { return false; }

    // Shy, Locked, Visible are not applicable
    bool IsShy() const override { return false; }
    void SetShy(bool) override {}
    bool IsLocked() const override { return false; }
    void SetLocked(bool) override {}
    bool IsVisible() const override { return true; }
    void SetVisible(bool) override {}

    // Keyframes, not applicable to a Slide
    void InsertKeyframe() override {}
    void DeleteAllChannelKeyframes() override {}
    IKeyframe *GetKeyframeByTime(long) const override { return nullptr; }

    // Keyframe manipulation, not applicable
    bool HasDynamicKeyframes(long inTime) override
    {
        Q_UNUSED(inTime);
        return false;
    }
    void SetDynamicKeyframes(long inTime, bool inDynamic) override
    {
        Q_UNUSED(inTime);
        Q_UNUSED(inDynamic);
    }

protected:
    std::shared_ptr<qt3dsdm::ISignalConnection>
        m_Connection; // Callback when the Asset name changes

    bool AmITimeParent() const override { return true; }
};

#endif // INCLUDED_SLIDE_TIMELINEITEM_BINDING_H
