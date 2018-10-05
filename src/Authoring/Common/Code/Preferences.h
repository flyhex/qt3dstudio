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
#define INCLUDED_PREFERENCES_H

#include "CColor.h"
#include <QtXml/qdom.h>

class CPreferences
{
public:
    CPreferences();
    ~CPreferences();

    void SetStringValue(const QString &key, const QString &value, const QString &group = {});
    QString GetStringValue(const QString &key, const QString &defaultValue = {},
                           const QString &group = {});

    void SetLongValue(const QString &inKey, long inValue, const QString &group = {});
    long GetLongValue(const QString &inKey, long inDefaultValue = 0, const QString &group = {});

    void SetValue(const QString &inKey, bool inValue, const QString &group = {});
    bool GetValue(const QString &inKey, bool inDefaultValue, const QString &group = {});

    void SetValue(const QString &inKey, double inValue, const QString &group = {});
    double GetValue(const QString &inKey, double inDefaultValue, const QString &group = {});

    void SetColorValue(const QString &inKey, ::CColor inValue, const QString &group = {});
    ::CColor GetColorValue(const QString &inKey, ::CColor inDefaultValue,
                           const QString &group = {});

    void SetPreferencesFile(const QString &inFileName);
    void save();

private:
    QString getValue(const QString &key, const QString &group = {});
    void setValue(const QString &key, const QString &value, const QString &group = {});

    QString m_PreferencesFile = {};
    QDomDocument m_domDoc = {};
};
#endif // INCLUDED_PREFERENCES_H
