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

#include "Qt3DSCommonPrecompile.h"
#include "Preferences.h"
#include "PreferencesSerializer.h"
#include "StringTokenizer.h"

CPreferencesSerializer CPreferences::s_PreferencesSerializer;

//=============================================================================
/**
 * Copy constructor.
 */
CPreferences::CPreferences(const CPreferences &inPrefs)
{
    m_TagPath = inPrefs.m_TagPath;
}

CPreferences::~CPreferences()
{
}

CPreferences &CPreferences::operator=(const CPreferences &inPrefs)
{
    if (&inPrefs != this) {
        m_TagPath = inPrefs.m_TagPath;
    }
    return *this;
}

//=============================================================================
/**
 * Sets the preferences serialization file
 * This sets the applications base path for all preferences that are to be
 * loaded. This should be called before any CPreferences are created.
 * @param inFileName preferences serialization file.
 */
void CPreferences::SetPreferencesFile(const QString &inFileName)
{
    s_PreferencesSerializer.SetPreferencesFile(inFileName);
}

//=============================================================================
/**
 * Get the User Preferences for this application.
 * This opens the Registry from HKEY_CURRENT_USER with key name set in
 * SetRegistryRoot.
 * Any values are in the user specific area of the registry.
 * @return the user preferences.
 */
CPreferences CPreferences::GetUserPreferences()
{
    return CPreferences();
}

//=============================================================================
/**
 * Get the User Preferences for this this application.
 * This opens the Registry from HKEY_CURRENT_USER with the key as inLocation
 * appended to the Registry Root.
 * Any values are in the user specific area of the registry.
 * @param inLocation appended to RegistryRoot to get the sub key location.
 * @return the user preferences.
 */
CPreferences CPreferences::GetUserPreferences(const QString &inLocation)
{
    return CPreferences(inLocation);
}

//=============================================================================
/**
 * Set the value of inKey to inValue.
 * @param inKey the name of the key to set.
 * @param inValue the value for the key.
 */
void CPreferences::SetStringValue(const QString &inKey,
                                  const QString &inValue)
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    s_PreferencesSerializer.SetSubElemValue(inKey, inValue);
}

//=============================================================================
/**
 * Get the value of inKey.
 * @param inKey the name of the key to get.
 * @param inDefaultValue the value to return if inKey's value cannot be gotten.
 * @return the value of inKey or inDefaultValue if an error ocurred.
 */
QString CPreferences::GetStringValue(const QString &inKey,
                                     const QString &inDefaultValue)
{
    QString theValue;
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    if (!s_PreferencesSerializer.GetSubElemValue(inKey, theValue))
        theValue = inDefaultValue;

    return theValue;
}

//=============================================================================
/**
 * Set the value of inKey to inValue.
 * @param inKey the name of the key to set.
 * @param inValue the value for the key.
 */
void CPreferences::SetLongValue(const QString &inKey, long inValue)
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    s_PreferencesSerializer.SetSubElemValue(inKey, QString::number(inValue));
}

//=============================================================================
/**
 * Get the value of inKey.
 * @param inKey the name of the key to get.
 * @param inDefaultValue the value to return if inKey's value cannot be gotten.
 * @return the value of inKey or inDefaultValue if an error occurred.
 */
long CPreferences::GetLongValue(const QString &inKey, long inDefaultValue)
{
    long theValue;
    QString theStrValue = GetStringValue(inKey);
    if (theStrValue.isEmpty())
        theValue = inDefaultValue;
    else
        theValue = theStrValue.toLong();

    return theValue;
}

//=============================================================================
/**
 * Set the value of inKey to inValue.
 * @param inKey the name of the key to set.
 * @param inValue the value for the key.
 */
void CPreferences::SetValue(const QString &inKey, bool inValue)
{
    long theRegValue = inValue ? 1 : 0;

    SetLongValue(inKey, theRegValue);
}

//=============================================================================
/**
 * Get the value of inKey.
 * @param inKey the name of the key to get.
 * @param inDefaultValue the value to return if inKey's value cannot be gotten.
 * @return the value of inKey or inDefaultValue if an error occurred.
 */
bool CPreferences::GetValue(const QString &inKey, bool inDefaultValue)
{
    long theDefaultValue = inDefaultValue ? 1 : 0;
    long theRegValue = GetLongValue(inKey, theDefaultValue);

    return theRegValue ? true : false;
}

//=============================================================================
/**
 * Set the value of inKey to inValue.
 * @param inKey the name of the key to set.
 * @param inValue the value for the key.
 */
void CPreferences::SetValue(const QString &inKey, double inValue)
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    s_PreferencesSerializer.SetSubElemValue(inKey, QString::number(inValue, 'g', 2));
}

//=============================================================================
/**
 * Get the value of inKey.
 * @param inKey the name of the key to get.
 * @param inDefaultValue the value to return if inKey's value cannot be gotten.
 * @return the value of inKey or inDefaultValue if an error occurred.
 */
double CPreferences::GetValue(const QString &inKey, double inDefaultValue)
{
    double theValue;
    QString theStrValue = GetStringValue(inKey);
    if (theStrValue.isEmpty())
        theValue = inDefaultValue;
    else
        theValue = theStrValue.toDouble();

    return theValue;
}

//=============================================================================
/**
 * Get the value of inKey as a color.
 * @param inKey the name of the key to get.
 * @param inDefaultColor the value to return if inKey's value cannot be gotten.
 * @return the value of inKey or inDefaultColor if an error occurred.
 */
CColor CPreferences::GetColorValue(const QString &inKey, CColor inDefaultColor)
{
    CColor theRetColor = inDefaultColor;
    QString theColorString = GetStringValue(inKey);
    if (!theColorString.isEmpty()) {
        const QStringList tokens = theColorString.split(QStringLiteral(" "));
        const QString &theR = tokens[0];
        const QString &theG = tokens[1];
        const QString &theB = tokens[2];
        theRetColor = ::CColor(theR.toLong(), theG.toLong(), theB.toLong());
    }
    return theRetColor;
}

//=============================================================================
/**
 * Set the value of inKey to inValue.
 * @param inKey the name of the key to set.
 * @param inValue the value for the key.
 */
void CPreferences::SetColorValue(const QString &inKey, CColor inValue)
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    QString theStrValue;
    QTextStream stream(&theStrValue);
    stream << QString::number(inValue.GetRed()) << QStringLiteral(" ")
           << QString::number(inValue.GetGreen()) << QStringLiteral(" ")
           << QString::number(inValue.GetBlue());
    s_PreferencesSerializer.SetSubElemValue(inKey, theStrValue);
}

void CPreferences::Clear()
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Remove(m_TagPath);
}

long CPreferences::GetItemCount()
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    return s_PreferencesSerializer.CountSubElems();
}

//====================================================================
/**
 * 	removes the specified sub element
 *	@param	inKeyName	the name of the sub element to be removed
 */
void CPreferences::RemoveKey(const QString &inKeyName)
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    s_PreferencesSerializer.RemoveSubElem(inKeyName);
}

//====================================================================
/**
 * 	Determines if the key exists
 *	@param	inKeyName	the name of the subkey
 */
bool CPreferences::Exists(const QString &inKeyName)
{
    s_PreferencesSerializer.Revert();
    s_PreferencesSerializer.Begin(m_TagPath);
    return s_PreferencesSerializer.ExistElem(inKeyName);
}
