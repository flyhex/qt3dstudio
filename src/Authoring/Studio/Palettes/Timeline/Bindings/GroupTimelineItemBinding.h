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
#ifndef INCLUDED_GROUP_TIMELINEITEM_BINDING_H
#define INCLUDED_GROUP_TIMELINEITEM_BINDING_H 1

#pragma once

#include "Qt3DSDMTimelineItemBinding.h"

//==============================================================================
//	Classes
//==============================================================================
class ITimelineItem;
class CTimelineTranslationManager;

//=============================================================================
/**
 * Binding to a DataModel object of Group type
 */
class CGroupTimelineItemBinding : public Qt3DSDMTimelineItemBinding
{
public:
    CGroupTimelineItemBinding(CTimelineTranslationManager *inMgr,
                              qt3dsdm::Qt3DSDMInstanceHandle inDataHandle);
    ~CGroupTimelineItemBinding() {}

    // Qt3DSDMTimelineItemBinding
    bool OpenAssociatedEditor() override;
    bool IsImported() const override;
};

#endif // INCLUDED_GROUP_TIMELINEITEM_BINDING_H
