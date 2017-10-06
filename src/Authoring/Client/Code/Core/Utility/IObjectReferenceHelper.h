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
#ifndef INCLUDED_IOBJECT_REFERENCE_HELPER_H
#define INCLUDED_IOBJECT_REFERENCE_HELPER_H 1

#pragma once

#include "RelativePathTools.h"
#include "StudioObjectTypes.h"
#include "UICDMHandles.h"
#include "UICDMValue.h"

//==============================================================================
/**
 *	Interface to abstract data model specific logic.
 */
class IObjectReferenceHelper
{
public:
    struct SObjectRefInfo
    {
        Q3DStudio::CString m_Name;
        EStudioObjectType m_Type;
        bool m_Master;

        SObjectRefInfo(Q3DStudio::CString inName, EStudioObjectType inType, bool inMaster)
            : m_Name(inName)
            , m_Type(inType)
            , m_Master(inMaster)
        {
        }
        SObjectRefInfo()
            : m_Type(OBJTYPE_UNKNOWN)
            , m_Master(false)
        {
        }
    };

public:
    virtual ~IObjectReferenceHelper() {}

public:
    virtual SObjectRefInfo GetInfo(const UICDM::CUICDMInstanceHandle &inInstance) const = 0;

    virtual UICDM::TSlideHandleList
    GetSlideList(const UICDM::CUICDMInstanceHandle inInstance) const = 0;
    virtual bool
    GetChildInstanceList(const UICDM::CUICDMInstanceHandle &inInstance,
                         UICDM::TInstanceHandleList &outlist, UICDM::CUICDMSlideHandle inSlide,
                         const UICDM::CUICDMInstanceHandle &inOwningInstance) const = 0;
    virtual Q3DStudio::CString
    LookupObjectFormalName(const UICDM::CUICDMInstanceHandle inInstance) const = 0;

    virtual Q3DStudio::CString
    GetObjectReferenceString(const UICDM::CUICDMInstanceHandle &inBaseInstance,
                             CRelativePathTools::EPathType inPathType,
                             const UICDM::CUICDMInstanceHandle &inInstance) const = 0;
    virtual bool ResolvePath(const UICDM::CUICDMInstanceHandle &inInstance,
                             const Q3DStudio::CString &inPathValue,
                             CRelativePathTools::EPathType &outType,
                             UICDM::CUICDMInstanceHandle &outResolvedInstance) = 0;
    virtual UICDM::CUICDMInstanceHandle
    Resolve(const UICDM::SValue &inObjectRefValue,
            const UICDM::CUICDMInstanceHandle &inBaseInstance) const = 0;
    virtual UICDM::SObjectRefType
    GetAssetRefValue(const UICDM::CUICDMInstanceHandle &inInstance,
                     const UICDM::CUICDMInstanceHandle &inBaseInstance,
                     CRelativePathTools::EPathType inPathType) const = 0;
};

#endif
