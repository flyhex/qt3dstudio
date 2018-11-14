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
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMValue.h"

//==============================================================================
/**
 *	Interface to abstract data model specific logic.
 */
class IObjectReferenceHelper
{
public:
    struct SObjectRefInfo
    {
        QString m_Name;
        EStudioObjectType m_Type;
        bool m_Master;

        SObjectRefInfo(const QString &inName, EStudioObjectType inType, bool inMaster)
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
    virtual SObjectRefInfo GetInfo(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance) const = 0;

    virtual qt3dsdm::TSlideHandleList
    GetSlideList(const qt3dsdm::Qt3DSDMInstanceHandle inInstance) const = 0;
    virtual bool
    GetChildInstanceList(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                         qt3dsdm::TInstanceHandleList &outlist, qt3dsdm::Qt3DSDMSlideHandle inSlide,
                         const qt3dsdm::Qt3DSDMInstanceHandle &inOwningInstance,
                         bool ignoreMaterialProperties = false) const = 0;
    virtual QString
    LookupObjectFormalName(const qt3dsdm::Qt3DSDMInstanceHandle inInstance) const = 0;

    virtual QString
    GetObjectReferenceString(const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance,
                             CRelativePathTools::EPathType inPathType,
                             const qt3dsdm::Qt3DSDMInstanceHandle &inInstance) const = 0;
    virtual bool ResolvePath(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                             const QString &inPathValue,
                             CRelativePathTools::EPathType &outType,
                             qt3dsdm::Qt3DSDMInstanceHandle &outResolvedInstance,
                             bool ignoreMaterialProperties = false) = 0;
    virtual qt3dsdm::Qt3DSDMInstanceHandle
    Resolve(const qt3dsdm::SValue &inObjectRefValue,
            const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance) const = 0;
    virtual qt3dsdm::SObjectRefType
    GetAssetRefValue(const qt3dsdm::Qt3DSDMInstanceHandle &inInstance,
                     const qt3dsdm::Qt3DSDMInstanceHandle &inBaseInstance,
                     CRelativePathTools::EPathType inPathType) const = 0;
};

#endif
