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

#ifndef INCLUDED_PREFERENCES_H
#define INCLUDED_PREFERENCES_H 1

#pragma once

#include "CColor.h"

//==============================================================================
//	Forwards
//==============================================================================
class CPreferencesSerializer;

class CPreferences
{
public:
    CPreferences(const Q3DStudio::CString &inTagPath)
        : m_TagPath(inTagPath)
    {
    }
    virtual ~CPreferences();

    CPreferences(const CPreferences &inPreferences);
    CPreferences &operator=(const CPreferences &inRightHandSide);

    static CPreferences GetUserPreferences();
    static CPreferences GetUserPreferences(const Q3DStudio::CString &inSubKeyPath);

    CPreferences GetPreferences(const Q3DStudio::CString &inPreferencesKey);

    void SetStringValue(const Q3DStudio::CString &inKey, const Q3DStudio::CString &inValue);
    Q3DStudio::CString GetStringValue(const Q3DStudio::CString &inKey,
                                      const Q3DStudio::CString &inDefaultValue);

    void SetLongValue(const Q3DStudio::CString &inKey, long inValue);
    long GetLongValue(const Q3DStudio::CString &inKey, long inDefaultValue);

    void SetValue(const Q3DStudio::CString &inKey, bool inValue);
    bool GetValue(const Q3DStudio::CString &inKey, bool inDefaultValue);

    void SetValue(const Q3DStudio::CString &inKey, double inValue);
    double GetValue(const Q3DStudio::CString &inKey, double inDefaultValue);

    void SetColorValue(const Q3DStudio::CString &inKey, ::CColor inValue);
    ::CColor GetColorValue(const Q3DStudio::CString &inKey, ::CColor inDefaultValue);

    void SetBinaryValue(const Q3DStudio::CString &inKey, const void *inData, long inLength);
    bool GetBinaryValue(const Q3DStudio::CString &inKey, void *outData, long inLength);

    void Clear();

    long GetItemCount();

    void Commit();

    void RemoveKey(const Q3DStudio::CString &inKeyName);

    bool Exists(const Q3DStudio::CString &inKeyName);

    static void SetPreferencesFile(const Q3DStudio::CString &inFileName);

private:
    Q3DStudio::CString m_TagPath;
    static CPreferencesSerializer s_PreferencesSerializer;
};
#endif // INCLUDED_PREFERENCES_H
