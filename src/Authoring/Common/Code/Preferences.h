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
    CPreferences(const QString &inTagPath = QString())
        : m_TagPath(inTagPath)
    {
    }
    virtual ~CPreferences();

    CPreferences(const CPreferences &inPreferences);
    CPreferences &operator=(const CPreferences &inRightHandSide);

    static CPreferences GetUserPreferences();
    static CPreferences GetUserPreferences(const QString &inSubKeyPath);

    CPreferences GetPreferences(const QString &inPreferencesKey);

    void SetStringValue(const QString &inKey, const QString &inValue);
    QString GetStringValue(const QString &inKey,
                           const QString &inDefaultValue = QString());

    void SetLongValue(const QString &inKey, long inValue);
    long GetLongValue(const QString &inKey, long inDefaultValue);

    void SetValue(const QString &inKey, bool inValue);
    bool GetValue(const QString &inKey, bool inDefaultValue);

    void SetValue(const QString &inKey, double inValue);
    double GetValue(const QString &inKey, double inDefaultValue);

    void SetColorValue(const QString &inKey, ::CColor inValue);
    ::CColor GetColorValue(const QString &inKey, ::CColor inDefaultValue);

    void SetBinaryValue(const QString &inKey, const void *inData, long inLength);
    bool GetBinaryValue(const QString &inKey, void *outData, long inLength);

    void Clear();

    long GetItemCount();

    void Commit();

    void RemoveKey(const QString &inKeyName);

    bool Exists(const QString &inKeyName);

    static void SetPreferencesFile(const QString &inFileName);

private:
    QString m_TagPath;
    static CPreferencesSerializer s_PreferencesSerializer;
};
#endif // INCLUDED_PREFERENCES_H
