/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_ITIMELINE_ITEM_PROPERTY_H
#define INCLUDED_ITIMELINE_ITEM_PROPERTY_H 1

#pragma once

#include "IKeyframeSelector.h"
#include "UICDMMetaData.h"
#include "UICString.h"

class CPropertyRow;
class IKeyframe;
class ITimelineKeyframesManager;

//=============================================================================
/**
 * Abstraction of a data model item's property that is displayed in the Timeline.
 */
//=============================================================================
class ITimelineItemProperty : public IKeyframeSelector
{
public:
    virtual ~ITimelineItemProperty() {}

    virtual Q3DStudio::CString GetName() const = 0;
    virtual bool IsMaster() const = 0;
    virtual UICDM::TDataTypePair GetType() const = 0;
    virtual float GetMaximumValue() const = 0;
    virtual float GetMinimumValue() const = 0;

    virtual void SetSelected() = 0;
    virtual void ClearKeySelection() = 0;
    virtual void DeleteAllKeys() = 0;

    virtual void Bind(CPropertyRow *inRow) = 0;
    virtual void Release() = 0;
    virtual CPropertyRow *GetRow() = 0;

    // Keyframes
    virtual ITimelineKeyframesManager *GetKeyframesManager() const = 0;
    virtual IKeyframe *GetKeyframeByTime(long inTime) const = 0;
    virtual IKeyframe *GetKeyframeByIndex(long inIndex) const = 0;
    virtual long GetKeyframeCount() const = 0;
    virtual long GetChannelCount() const = 0;
    virtual float GetChannelValueAtTime(long inChannelIndex, long inTime) = 0;
    virtual void SetChannelValueAtTime(long inChannelIndex, long inTime, float inValue) = 0;
    virtual long OffsetSelectedKeyframes(long inOffset) = 0;
    virtual void CommitChangedKeyframes() = 0;
    virtual void OnEditKeyframeTime(long inCurrentTime, long inObjectAssociation) = 0;
    virtual bool IsDynamicAnimation() = 0;
};

#endif // INCLUDED_ITIMELINE_ITEM_H
