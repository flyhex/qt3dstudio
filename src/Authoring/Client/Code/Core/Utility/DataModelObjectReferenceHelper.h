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
// Prefix
//==============================================================================
#ifndef INCLUDED_OBJECT_REFERENCE_HELPER_H
#define INCLUDED_OBJECT_REFERENCE_HELPER_H 1

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "IObjectReferenceHelper.h"
#include "UICDMHandles.h"

//==============================================================================
/**
 *	Implementation of IObjectReferenceHelper
 *	For now, this cross from the old client to new UICDM boundaries, and shield that logic from
 *the actual control classes.
 *	Eventually, when all assets is converted to UICDM land, the meat of all these functions can
 *change accordingly.
 */
class CObjectReferenceHelper : public IObjectReferenceHelper
{
public:
    CObjectReferenceHelper(CDoc *inDoc);
    ~CObjectReferenceHelper();

    SObjectRefInfo GetInfo(const UICDM::CUICDMInstanceHandle &inInstance) const override;

    virtual UICDM::TSlideHandleList
    GetSlideList(const UICDM::CUICDMInstanceHandle inInstance) const override;
    bool GetChildInstanceList(const UICDM::CUICDMInstanceHandle &inInstance,
                                      UICDM::TInstanceHandleList &outList,
                                      UICDM::CUICDMSlideHandle inSlide,
                                      const UICDM::CUICDMInstanceHandle &inOwningInstance) const override;
    virtual Q3DStudio::CString
    LookupObjectFormalName(const UICDM::CUICDMInstanceHandle inInstance) const override;

    virtual Q3DStudio::CString
    GetObjectReferenceString(const UICDM::CUICDMInstanceHandle &inBaseInstance,
                             CRelativePathTools::EPathType inPathType,
                             const UICDM::CUICDMInstanceHandle &inInstance) const override;
    bool ResolvePath(const UICDM::CUICDMInstanceHandle &inInstance,
                             const Q3DStudio::CString &inPathValue,
                             CRelativePathTools::EPathType &outType,
                             UICDM::CUICDMInstanceHandle &outResolveInstance) override;
    virtual UICDM::CUICDMInstanceHandle
    Resolve(const UICDM::SValue &inObjectRefValue,
            const UICDM::CUICDMInstanceHandle &inBaseInstance) const override;

    virtual UICDM::SObjectRefType
    GetAssetRefValue(const UICDM::CUICDMInstanceHandle &inInstance,
                     const UICDM::CUICDMInstanceHandle &inBaseInstance,
                     CRelativePathTools::EPathType inPathType) const override;

protected: // UICDM
    void GetPropertyAsChildrenList(const UICDM::CUICDMInstanceHandle &inInstance,
                                   UICDM::TInstanceHandleList &outList, long inSlideIndex) const;

protected:
    CDoc *m_Doc;
};

#endif
