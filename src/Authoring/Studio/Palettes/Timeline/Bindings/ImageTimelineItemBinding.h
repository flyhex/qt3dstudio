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
#ifndef INCLUDED_IMAGE_TIMELINEITEM_BINDING_H
#define INCLUDED_IMAGE_TIMELINEITEM_BINDING_H 1

#pragma once

#include "Qt3DSDMTimelineItemBinding.h"

//==============================================================================
//	Classes
//==============================================================================
class CTimelineTranslationManager;
class CBaseStateRow;
class ITimelineTimebar;

//=============================================================================
/**
 * Binding to a UICDM object of Image type
 */
class CImageTimelineItemBinding : public Qt3DSDMTimelineItemBinding
{
public:
    CImageTimelineItemBinding(CTimelineTranslationManager *inMgr,
                              qt3dsdm::Qt3DSDMInstanceHandle inDataHandle);
    virtual ~CImageTimelineItemBinding();

    // Qt3DSDMTimelineItemBinding
    ITimelineTimebar *GetTimebar() override;
    Q3DStudio::CString GetName() const override;
    void SetName(const Q3DStudio::CString &inName) override;
    EStudioObjectType GetObjectType() const override;
    bool ShowToggleControls() const override;
    void Bind(CBaseStateRow *inRow) override;
    bool OpenAssociatedEditor() override;

    void SetPropertyHandle(qt3dsdm::Qt3DSDMPropertyHandle inProperty)
    {
        m_PropertyHandle = inProperty;
    }
    qt3dsdm::Qt3DSDMPropertyHandle GetPropertyHandle() const { return m_PropertyHandle; }

protected:
    Q3DStudio::CString m_Name;
    qt3dsdm::Qt3DSDMPropertyHandle m_PropertyHandle;
};

#endif // INCLUDED_IMAGE_TIMELINEITEM_BINDING_H
