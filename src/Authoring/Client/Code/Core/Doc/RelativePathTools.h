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

#ifndef INCLUDED_RELATIVEPATHTOOLS_H
#define INCLUDED_RELATIVEPATHTOOLS_H 1

#pragma once

#include "UICDMDataTypes.h"
#include "UICDMHandles.h"
#include "UICString.h"

class CAsset;
class CStackTokenizer;
class IObjectReferenceHelper;
class CDoc;

class CRelativePathTools
{
    //==============================================================================
    //	 Enumeration
    //==============================================================================
public:
    enum EPathType {
        EPATHTYPE_GUID = 0,
        EPATHTYPE_RELATIVE,
    };

    //==============================================================================
    //	 Static Methods
    //==============================================================================
public:
    static bool IsGUID(const qt3dsdm::SObjectRefType &inObjectRefValue);
    static bool IsRelativePath(const qt3dsdm::SObjectRefType &inObjectRefValue);
    static EPathType GetPathType(const qt3dsdm::SObjectRefType &inObjectRefValue);

    static Q3DStudio::CString BuildReferenceString(const qt3dsdm::CUICDMInstanceHandle inInstance,
                                                   const qt3dsdm::CUICDMInstanceHandle inRootInstance,
                                                   EPathType inPathType, CDoc *inDoc);
    static Q3DStudio::CString
    BuildAbsoluteReferenceString(const qt3dsdm::CUICDMInstanceHandle inInstance, CDoc *inDoc);
    static Q3DStudio::CString
    BuildRelativeReferenceString(const qt3dsdm::CUICDMInstanceHandle inInstance,
                                 const qt3dsdm::CUICDMInstanceHandle inRootInstance, CDoc *inDoc);
    static qt3dsdm::CUICDMInstanceHandle
    FindAssetInstanceByObjectPath(CDoc *inDoc, const qt3dsdm::CUICDMInstanceHandle &inRootInstance,
                                  const Q3DStudio::CString &inString, EPathType &outPathType,
                                  bool &outIsResolved,
                                  const IObjectReferenceHelper *inHelper = NULL);
    static qt3dsdm::SObjectRefType
    CreateAssetRefValue(const qt3dsdm::CUICDMInstanceHandle inInstance,
                        const qt3dsdm::CUICDMInstanceHandle inRootInstance, EPathType inPathType,
                        CDoc *inDoc);

protected:
    static qt3dsdm::CUICDMInstanceHandle
    DoFindAssetInstanceByObjectPath(CDoc *inDoc, const qt3dsdm::CUICDMInstanceHandle &inRootInstance,
                                    const qt3dsdm::CUICDMInstanceHandle inTimeParentInstance,
                                    qt3dsdm::CUICDMSlideHandle inSlide, CStackTokenizer &ioTokenizer,
                                    bool &outIsResolved, const IObjectReferenceHelper *inHelper);
    static Q3DStudio::CString LookupObjectName(const qt3dsdm::CUICDMInstanceHandle inInstance,
                                               CDoc *inDoc);
};

#endif // INCLUDED_RELATIVEPATHTOOLS_H
