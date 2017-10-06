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

//==============================================================================
//	Prefix
//==============================================================================

#ifndef INCLUDEED_UICID
#define INCLUDEED_UICID

#pragma once

//==============================================================================
//	Include
//==============================================================================

#include "UICString.h"
#include <map>
#ifdef _WIN32
#include <guiddef.h>
#else
#include "PlatformTypes.h"
#endif

namespace Q3DStudio {

typedef struct _TGuidStd
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} TGUIDStd;

typedef struct _TGuidPacked
{
    unsigned long Data1;
    unsigned long Data2;
    unsigned long Data3;
    unsigned long Data4;
} TGUIDPacked;

union UUID {
    // uuid_t leach;
    TGUIDStd GuidStd;
    TGUIDPacked GuidPacked;
};

// {E5D7768B-9111-45bc-98E6-9B0C44C0B1B6}
static const UUID UUID_ZERO = {{ 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }};

class CId
{
public:
    CId();
    CId(wchar_t *inStringId);
    CId(const CId &inId);
    CId(const GUID &inGUID);
    CId(long in1, long in2, long in3, long in4);
    void Generate();
    GUID Convert() const;

    bool operator==(const CId &inRVal) const;
    bool operator==(const Q3DStudio::UUID &inRVal) const;
    bool operator!=(const CId &inRVal) const;
    CId &operator=(const CId &inRVal);
    bool operator<(const CId &inRVal) const;
    bool IsZero() const;
    operator GUID() const;
    // Not perfected yet!

    Q3DStudio::CString ToString() const;
    CId &FromString(const Q3DStudio::CString &inStringId);

    operator TGUIDPacked &() { return m_Key.GuidPacked; }
    operator const TGUIDPacked &() const { return m_Key.GuidPacked; }

private:
    UUID m_Key;
};

typedef std::map<Q3DStudio::CId, Q3DStudio::CId> TIDIDMap;

} // End if namespace Q3DStudio
#endif
