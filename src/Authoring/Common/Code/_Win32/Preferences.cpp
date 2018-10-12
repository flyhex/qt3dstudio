/****************************************************************************
**
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

#include "Preferences.h"
#include <QtCore/qfile.h>

CPreferences::CPreferences()
{
}

CPreferences::~CPreferences()
{
    save();
}

void CPreferences::save()
{
    if (!m_PreferencesFile.isEmpty()) {
        QFile file(m_PreferencesFile);
        file.open(QIODevice::WriteOnly);
        file.resize(0);
        file.write(m_domDoc.toByteArray(4));
    }
}

/**
 * Sets the preferences file path
 * This sets the applications base path for all preferences that are to be
 * loaded. It also creates the preferences file if it doesn't exist. This
 *  should be called before any CPreferences are created.
 * @param filePath preferences serialization file path.
 */
void CPreferences::SetPreferencesFile(const QString &filePath)
{
    if (m_PreferencesFile == filePath)
        return;

    m_PreferencesFile = filePath;

    QFile file(m_PreferencesFile);
    if (!file.exists()) {
        m_domDoc.setContent(QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                                           "<Settings>"
                                           "</Settings>"));

        file.open(QIODevice::WriteOnly);
        file.resize(0);
        file.write(m_domDoc.toByteArray(4));
    } else {
        file.open(QIODevice::ReadOnly);
        m_domDoc.setContent(&file);
    }
}

void CPreferences::SetStringValue(const QString &key, const QString &value,
                                  const QString &group)
{
    setValue(key, value, group);
}

QString CPreferences::GetStringValue(const QString &key, const QString &defaultValue,
                                     const QString &group)
{
    QString value = getValue(key, group);
    return value.isEmpty() ? defaultValue : value;
}

void CPreferences::SetLongValue(const QString &key, long value,
                                const QString &group)
{
    setValue(key, QString::number(value), group);
}

long CPreferences::GetLongValue(const QString &key, long defaultValue,
                                const QString &group)
{
    QString theStrValue = GetStringValue(key, {}, group);
    return theStrValue.isEmpty() ? defaultValue : theStrValue.toLong();
}

void CPreferences::SetValue(const QString &key, bool value, const QString &group)
{
    long theRegValue = value ? 1 : 0;
    SetLongValue(key, theRegValue, group);
}

bool CPreferences::GetValue(const QString &key, bool defaultValue, const QString &group)
{
    long theDefaultValue = defaultValue ? 1 : 0;
    long theRegValue = GetLongValue(key, theDefaultValue, group);

    return theRegValue ? true : false;
}

void CPreferences::SetValue(const QString &key, double value, const QString &group)
{
    setValue(key, QString::number(value), group);
}

double CPreferences::GetValue(const QString &key, double defaultValue, const QString &group)
{
    QString theStrValue = GetStringValue(key, {}, group);
    return theStrValue.isEmpty() ? defaultValue : theStrValue.toDouble();
}

CColor CPreferences::GetColorValue(const QString &key, CColor defaultColor, const QString &group)
{
    QString theColorString = GetStringValue(key, {}, group);
    if (!theColorString.isEmpty()) {
        QStringList rgb = theColorString.split(QStringLiteral(" "));
        return ::CColor(rgb.at(0).toInt(), rgb.at(1).toInt(), rgb.at(2).toInt());
    }

    return defaultColor;
}

void CPreferences::SetColorValue(const QString &key, CColor value, const QString &group)
{
    QString rgbStr = QString("%1 %2 %2").arg(value.red).arg(value.green).arg(value.blue);
    setValue(key, rgbStr, group);
}

QString CPreferences::getValue(const QString &key, const QString &group)
{
    if (m_PreferencesFile.isEmpty())
        return {};

    QDomElement parentElem = group.isEmpty() ? m_domDoc.documentElement()
                                             : m_domDoc.documentElement().firstChildElement(group);
    if (!parentElem.isNull()) {
        QDomNodeList itemNodes = parentElem.elementsByTagName(QStringLiteral("Item"));
        if (!itemNodes.isEmpty()) {
            for (int i = 0; i < itemNodes.count(); ++i) {
                QDomElement itemElem = itemNodes.at(i).toElement();
                if (itemElem.attribute(QStringLiteral("Name")) == key)
                    return itemElem.attribute(QStringLiteral("value"));
            }
        }
    }

    return {};
}

void CPreferences::setValue(const QString &key, const QString &value,
                            const QString &group)
{
    if (m_domDoc.isNull())
        return;

    QDomElement parentElem = group.isEmpty() ? m_domDoc.documentElement()
                                             : m_domDoc.documentElement().firstChildElement(group);
    if (!parentElem.isNull()) {
        QDomNodeList itemNodes = parentElem.elementsByTagName(QStringLiteral("Item"));
        if (!itemNodes.isEmpty()) {
            for (int i = 0; i < itemNodes.count(); ++i) {
                QDomElement itemElem = itemNodes.at(i).toElement();
                if (itemElem.attribute(QStringLiteral("Name")) == key) {
                    // property exist, update it
                    itemElem.setAttribute(QStringLiteral("value"), value);
                    return;
                }
            }
        }

        // if property doesn't exist, create a new one
        QDomElement elem = m_domDoc.createElement("Item");
        elem.setAttribute(QStringLiteral("Name"), key);
        elem.setAttribute(QStringLiteral("value"), value);
        parentElem.appendChild(elem);
    }
}
