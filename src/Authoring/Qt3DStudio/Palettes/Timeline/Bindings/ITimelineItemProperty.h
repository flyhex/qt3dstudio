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
#define INCLUDED_ITIMELINE_ITEM_PROPERTY_H

#include "Qt3DSDMMetaData.h"
#include "Qt3DSString.h"
#include "Qt3DSDMAnimation.h"

class RowTree;
class IKeyframe;

//=============================================================================
/**
 * Abstraction of a data model item's property that is displayed in the Timeline.
 */
//=============================================================================
class ITimelineItemProperty
{
public:
    virtual ~ITimelineItemProperty() {}

    virtual Q3DStudio::CString GetName() const = 0;
    virtual bool IsMaster() const = 0;
    virtual qt3dsdm::TDataTypePair GetType() const = 0;

    virtual void SetSelected() = 0;
    virtual void DeleteAllKeys() = 0;

    virtual void setRowTree(RowTree *row) = 0;
    virtual RowTree *getRowTree() const = 0;

    virtual qt3dsdm::Qt3DSDMPropertyHandle getPropertyHandle() const = 0;
    virtual std::vector<qt3dsdm::Qt3DSDMAnimationHandle> animationHandles() const = 0;
    virtual qt3dsdm::EAnimationType animationType() const = 0;

    // Keyframes
    virtual IKeyframe *GetKeyframeByTime(long inTime) const = 0;
    virtual IKeyframe *GetKeyframeByIndex(long inIndex) const = 0;
    virtual long GetKeyframeCount() const = 0;
    virtual size_t GetChannelCount() const = 0;
    virtual float GetChannelValueAtTime(size_t chIndex, long time) = 0;
    virtual bool IsDynamicAnimation() = 0;
};

#endif // INCLUDED_ITIMELINE_ITEM_H
