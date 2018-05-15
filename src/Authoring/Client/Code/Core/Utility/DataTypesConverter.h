/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMMetaData.h"
#include "Vector2.h"
#include "Rotation3.h"
#include "Vector3.h"
#include "Qt3DSVec2.h"
#include "Qt3DSVec3.h"

// Convenience functions to convert from CoreLibProject/Types Files to UICDMDataTypes
inline qt3dsdm::SValue ConvertToSValue(const Q3DStudio::CVector2 &inValue)
{
    qt3dsdm::SFloat2 theFloat2;
    theFloat2.m_Floats[0] = inValue.GetX();
    theFloat2.m_Floats[1] = inValue.GetY();
    return (qt3dsdm::SValue(theFloat2));
}
inline qt3dsdm::SValue ConvertToSValue(const Q3DStudio::CVector3 &inValue)
{
    qt3dsdm::SFloat3 theFloat3;
    theFloat3.m_Floats[0] = inValue.GetX();
    theFloat3.m_Floats[1] = inValue.GetY();
    theFloat3.m_Floats[2] = inValue.GetZ();
    return (qt3dsdm::SValue(theFloat3));
}

inline qt3dsdm::SValue ConvertToSValue(const Q3DStudio::CRotation3 &inValue)
{
    qt3dsdm::SFloat3 theFloat3;
    theFloat3.m_Floats[0] = inValue.GetXDegrees();
    theFloat3.m_Floats[1] = inValue.GetYDegrees();
    theFloat3.m_Floats[2] = inValue.GetZDegrees();
    return (qt3dsdm::SValue(theFloat3));
}

inline qt3dsdm::SValue ConvertToSValue(float inX, float inY)
{
    return (qt3dsdm::SValue(qt3dsdm::SFloat2(inX, inY)));
}

inline qt3dsdm::SValue ConvertToSValue(float inX, float inY, float inZ)
{
    return (qt3dsdm::SValue(qt3dsdm::SFloat3(inX, inY, inZ)));
}

inline qt3dsdm::SValue ConvertToSValue(const qt3ds::QT3DSVec2 &inValue)
{
    qt3dsdm::SFloat2 theFloat2;
    theFloat2.m_Floats[0] = inValue.x;
    theFloat2.m_Floats[1] = inValue.y;
    return (qt3dsdm::SValue(theFloat2));
}

inline qt3dsdm::SValue ConvertToSValue(const qt3ds::QT3DSVec3 &inValue)
{
    qt3dsdm::SFloat3 theFloat3;
    theFloat3.m_Floats[0] = inValue.x;
    theFloat3.m_Floats[1] = inValue.y;
    theFloat3.m_Floats[2] = inValue.z;
    return (qt3dsdm::SValue(theFloat3));
}

inline qt3dsdm::SValue ConvertToSValue(const wchar_t *inValue)
{
    return qt3dsdm::TDataStrPtr(new qt3dsdm::CDataStr(inValue));
}

inline qt3dsdm::SValue ConvertToSValue(const std::wstring &inValue)
{
    return qt3dsdm::TDataStrPtr(new qt3dsdm::CDataStr(inValue.c_str()));
}

inline qt3dsdm::SValue ConvertToSValue(const char *inValue)
{
    return qt3dsdm::TDataStrPtr(new qt3dsdm::CDataStr(Q3DStudio::CString(inValue)));
}

inline qt3dsdm::SValue ConvertToSValue(const Q3DStudio::CString &inValue)
{
    return qt3dsdm::TDataStrPtr(new qt3dsdm::CDataStr(inValue));
}

inline qt3dsdm::TMetaDataStringList ConvertToStringList(Q3DStudio::CString inValue)
{
    // Convert the incoming string attribute into a list
    qt3dsdm::TMetaDataStringList theList;

    Q3DStudio::CString theDelimiter(QStringLiteral(","));
    long theStartIndex = 0;
    long theEndIndex = theStartIndex;
    for (;;) {
        theEndIndex = inValue.Find(theDelimiter, theStartIndex);
        if (theEndIndex == Q3DStudio::CString::ENDOFSTRING)
            break;
        theList.push_back(inValue.Extract(theStartIndex, theEndIndex - theStartIndex).Unique());
        theStartIndex = theEndIndex + 1;
    }
    if (inValue.Length() > 0) {
        theList.push_back(inValue.Extract(theStartIndex).Unique());
    }

    return theList;
}

inline Q3DStudio::CString ConvertFromStringList(qt3dsdm::TMetaDataStringList inList)
{
    size_t theNumListItems = inList.size();
    Q3DStudio::CString theData = "";
    if (theNumListItems > 0) {
        for (qt3dsdm::TMetaDataStringList::iterator theIterator = inList.begin();
             theIterator != inList.end(); ++theIterator) {
            theData += theIterator->c_str();
            if (theIterator + 1 != inList.end())
                theData += QStringLiteral(",");
        }
    }
    return theData;
}
