/****************************************************************************
**
** Copyright (C) 1999-2007 NVIDIA Corporation.
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
#ifndef INCLUDED_BUILD_CONFIG_PARSER_H
#define INCLUDED_BUILD_CONFIG_PARSER_H

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include <list>

#include <QMetaType>
#include "UICFile.h"
#include <QtCore/qxmlstream.h>


namespace Q3DStudio {

//=============================================================================
/**
 *	Wrapper class to store a Build configuration.
 */
class CBuildConfiguration
{
public:
    struct SConfigPropertyValue
    {
    protected:
        CString m_Name; ///< name of property value
        CString m_Label; ///< display name of property value

    public:
        const CString &GetName() { return m_Name; }
        void SetName(const CString &inName) { m_Name = inName; }

        const CString &GetLabel() { return m_Label; }
        void SetLabel(const CString &inLabel) { m_Label = inLabel; }
    };

    typedef std::vector<SConfigPropertyValue> TConfigPropertyValues; ///< List of property values
    struct SConfigProperty
    {
    protected:
        CString m_Name; ///< name of property
        CString m_Label; ///< display name of property
        CString m_Help; ///< help string of property
        TConfigPropertyValues m_AcceptableValues; ///< List of acceptable values

    public:
        const CString &GetName() { return m_Name; }
        void SetName(const CString &inName) { m_Name = inName; }

        const CString &GetLabel() { return m_Label; }
        void SetLabel(const CString &inLabel) { m_Label = inLabel; }

        const CString &GetHelp() { return m_Help; }
        void SetHelp(const CString &inHelp) { m_Help = inHelp; }

        void AddValue(SConfigPropertyValue &inValue) { m_AcceptableValues.push_back(inValue); }

        bool HasValue(const CString &inPropertyValue)
        {
            TConfigPropertyValues::iterator theIter;
            for (theIter = m_AcceptableValues.begin(); theIter != m_AcceptableValues.end();
                 ++theIter) {
                if (theIter->GetName() == inPropertyValue)
                    return true;
            }
            return false;
        }

        SConfigPropertyValue &GetValue(long inIndex) { return m_AcceptableValues.at(inIndex); }

        TConfigPropertyValues &GetAcceptableValues() { return m_AcceptableValues; }
    };

    typedef std::list<SConfigProperty> TConfigProperties; ///< List of properties

protected:
    CString m_Name; ///< Name of this configuration
    CString m_Path; ///< Absolute path of this configuration
    CString m_PreviewApp; ///< preview application syntax
    CString m_DeployApp; ///< delploy application syntax
    TConfigProperties m_ConfigProperties; ///< List of properties

public:
    CBuildConfiguration(const CString &inPath)
        : m_Path(inPath)
    {
    }

    const CString &GetPath() { return m_Path; }
    void SetPath(const CString &inPath) { m_Path = inPath; }

    const CString &GetName() { return m_Name; }
    void SetName(const CString &inName) { m_Name = inName; }

    const CString &GetPreviewApp() { return m_PreviewApp; }
    void SetPreviewApp(const CString &inPreviewApp) { m_PreviewApp = inPreviewApp; }

    const CString &GetDeployApp() { return m_DeployApp; }
    void SetDeployApp(const CString &inDeployApp) { m_DeployApp = inDeployApp; }

    SConfigProperty &AddProperty(SConfigProperty &inProperty)
    {
        m_ConfigProperties.push_back(inProperty);
        return m_ConfigProperties.back();
    }

    SConfigProperty *GetProperty(const CString &inPropertyName)
    {
        TConfigProperties::iterator theIter;
        for (theIter = m_ConfigProperties.begin(); theIter != m_ConfigProperties.end(); ++theIter) {
            if (theIter->GetName() == inPropertyName)
                return &*theIter;
        }

        return nullptr;
    }

    TConfigProperties &GetBuildProperties() { return m_ConfigProperties; }
};

//=============================================================================
/**
 *	Wrapper class to store a list of Build configurations.
 */
class CBuildConfigurations
{
public:
    typedef std::map<CString, CBuildConfiguration *>
        TBuildConfigurations; ///< List of build configurations, sorted by name

public:
    void AddConfiguration(CBuildConfiguration *inConfig)
    {
        m_BuildConfigurations.insert(std::make_pair(inConfig->GetName(), inConfig));
    }

    CBuildConfiguration *GetConfiguration(const CString &inName)
    {
        TBuildConfigurations::iterator theFind = m_BuildConfigurations.find(inName);
        if (theFind != m_BuildConfigurations.end())
            return theFind->second;
        else
            return NULL;
    }

    TBuildConfigurations &GetConfigurations() { return m_BuildConfigurations; }

    void Clear()
    {
        TBuildConfigurations::iterator theIter;
        for (theIter = m_BuildConfigurations.begin(); theIter != m_BuildConfigurations.end();
             ++theIter) {
            delete theIter->second;
        }
        m_BuildConfigurations.clear();
    }

protected:
    TBuildConfigurations m_BuildConfigurations; ///< list of build configurations
};

//=============================================================================
/**
 * Parser for build configurations (.build) files in "Build Configurations" folder
 */
class CBuildConfigParser
{
    CBuildConfigParser(const CBuildConfigParser &other);
    CBuildConfigParser &operator=(const CBuildConfigParser &other);

public:
    CBuildConfigParser(CBuildConfigurations &inConfigurations);
    virtual ~CBuildConfigParser();

public:
    bool LoadConfigurations(CUICFile &inDirectory);
    CString GetErrorMessage() { return CString::fromQString(m_ErrorMessage); }

protected:
    void ParseProjectAttributes(const QXmlStreamAttributes &inAttributes);
    void ParsePropertyAttributes(CBuildConfiguration::SConfigProperty &inProperty,
                                 const QXmlStreamAttributes &inAttributes);
    void ParseValueAttributes(const QXmlStreamAttributes &inAttributes);

protected:
    void StartElement(const QString &inElementName,
                      const QXmlStreamAttributes &inAttributes);
    void HandleCharacterData(const QString &data, int inLen);
    void EndElement(const QString &inElementName);
    void DocumentFinished();

protected:
    CBuildConfigurations &m_BuildConfigurations; ///< Working list of build configurations
    CBuildConfiguration *m_CurrentConfiguration; ///< Current parsing configuration
    CBuildConfiguration::SConfigProperty *m_CurrentProperty; ///< Current parsing property
    CString m_CurrentFile; ///< Current Parse file
    CString m_ElementData; ///< Element Data read in
    QString m_ErrorMessage; ///< Consolidated error messages
    bool m_TagStarted; ///< Hacked to signify tag started
};

} // namespace Q3DStudio

Q_DECLARE_METATYPE(Q3DStudio::CBuildConfiguration *)
Q_DECLARE_METATYPE(Q3DStudio::CBuildConfiguration::SConfigPropertyValue *)

#endif // INCLUDED_BUILD_CONFIG_PARSER_H
