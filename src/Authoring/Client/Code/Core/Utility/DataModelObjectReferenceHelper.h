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
#include "Qt3DSDMHandles.h"

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

    SObjectRefInfo GetInfo(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance) const override;

    virtual qt3dsdm::TSlideHandleList
    GetSlideList(const qt3dsdm::Qt3DSDMInstanceHandle inInstance) const override;
    bool GetChildInstanceList(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                                      qt3dsdm::TInstanceHandleList &outList,
                                      qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                      const qt3dsdm::Qt3DSDMInstanceHandle &inOwningInstance) const override;
    virtual Q3DStudio::CString
    LookupObjectFormalName(const qt3dsdm::Qt3DSDMInstanceHandle inInstance) const override;

    virtual Q3DStudio::CString
    GetObjectReferenceString(const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance,
                             CRelativePathTools::EPathType inPathType,
                             const qt3dsdm::Qt3DSDMInstanceHandle &inInstance) const override;
    bool ResolvePath(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                             const Q3DStudio::CString &inPathValue,
                             CRelativePathTools::EPathType &outType,
                             qt3dsdm::Qt3DSDMInstanceHandle &outResolveInstance) override;
    virtual qt3dsdm::Qt3DSDMInstanceHandle
    Resolve(const qt3dsdm::SValue &inObjectRefValue,
            const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance) const override;

    virtual qt3dsdm::SObjectRefType
    GetAssetRefValue(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                     const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance,
                     CRelativePathTools::EPathType inPathType) const override;

protected: // UICDM
    void GetPropertyAsChildrenList(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                                   qt3dsdm::TInstanceHandleList &outList, long inSlideIndex) const;

protected:
    CDoc *m_Doc;
};

#endif
