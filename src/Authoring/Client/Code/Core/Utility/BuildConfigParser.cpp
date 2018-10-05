/****************************************************************************
**
** Copyright (C) 1999-2006 NVIDIA Corporation.
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

#include "BuildConfigParser.h"
#include <QtCore/qdiriterator.h>

namespace Q3DStudio {

const auto STR_TAG_PROJECT = QStringLiteral("project");
const auto STR_TAG_INTEGRATION = QStringLiteral("uic:integration");
const auto STR_TAG_PREVIEW = QStringLiteral("uic:preview");
const auto STR_TAG_DEPLOY = QStringLiteral("uic:deploy");
const auto STR_TAG_PROPERTY = QStringLiteral("uic:property");
const auto STR_TAG_VALUE = QStringLiteral("uic:value");

const auto STR_PARAM_NAME = QStringLiteral("name");
const auto STR_PARAM_LABEL = QStringLiteral("label");
const auto STR_PARAM_HELP = QStringLiteral("help");

CBuildConfigParser::CBuildConfigParser(CBuildConfigurations &inConfigurations)
    : m_BuildConfigurations(inConfigurations)
    , m_CurrentConfiguration(nullptr)
    , m_CurrentProperty(nullptr)
    , m_TagStarted(false)
{
}

CBuildConfigParser::~CBuildConfigParser()
{
}

/**
 *	Load all the build configuration files in the directory.
 *	@param inDirectory		the directory to look for all the configuration files
 *	@return true if all files parsed successfully, else false
 */
bool CBuildConfigParser::LoadConfigurations(const QString &configDirPath)
{
    m_ErrorMessage.clear();

    QDirIterator di(configDirPath, QDir::NoDotAndDotDot | QDir::Files);
    while (di.hasNext()) {
        QFileInfo fi = di.next();
        if (fi.suffix() == QLatin1String("build")) {
            m_CurrentConfiguration = new CBuildConfiguration(fi.absoluteFilePath());
            QFile file(fi.absoluteFilePath());
            file.open(QFile::ReadOnly);
            QXmlStreamReader reader(&file);
            reader.setNamespaceProcessing(false);
            while (!reader.atEnd()) {
                QXmlStreamReader::TokenType token = reader.readNext();
                if (token == QXmlStreamReader::StartElement)
                    StartElement(reader.qualifiedName().toString(), reader.attributes());
                else if (token == QXmlStreamReader::Characters)
                    HandleCharacterData(reader.text().toString(), 0);
                else if (token == QXmlStreamReader::EndElement)
                    EndElement(reader.qualifiedName().toString());
            }
            if (reader.hasError()) {
                m_ErrorMessage += reader.errorString();
                delete m_CurrentConfiguration;
                m_CurrentConfiguration = nullptr;
            } else {
                DocumentFinished();
            }
            file.close();
        }
    }


    // Check the error message to see if parse successful or fail
    if (m_ErrorMessage.isEmpty())
        return true;
    else
        return false;
}


/**
 *	Callback when completed the parsing. Add the build configuration loaded to the
 *	stored list
 */
void CBuildConfigParser::DocumentFinished()
{
    if (m_CurrentConfiguration) {
        m_BuildConfigurations.AddConfiguration(m_CurrentConfiguration);
        m_CurrentConfiguration = nullptr;
    }
}

/**
 *	Callback when starting parsing an element
 */
void CBuildConfigParser::StartElement(const QString &inElementName,
                                      const QXmlStreamAttributes &inAttributes)
{
    if (inElementName == STR_TAG_PROJECT) {
        ParseProjectAttributes(inAttributes);
    } else if (inElementName == STR_TAG_PROPERTY) {
        CBuildConfiguration::SConfigProperty theProperty;
        ParsePropertyAttributes(theProperty, inAttributes);
        m_CurrentProperty = &m_CurrentConfiguration->AddProperty(theProperty);
    } else if (inElementName == STR_TAG_VALUE) {
        ParseValueAttributes(inAttributes);
    } else if (inElementName == STR_TAG_PREVIEW || inElementName == STR_TAG_DEPLOY) {
        m_TagStarted = true;
        m_ElementData = "";
    }
}

/**
 *	Callback after finished parsing an element
 */
void CBuildConfigParser::EndElement(const QString &inElementName)
{
    if (inElementName == STR_TAG_PREVIEW)
        m_CurrentConfiguration->SetPreviewApp(m_ElementData);
    else if (inElementName == STR_TAG_DEPLOY)
        m_CurrentConfiguration->SetDeployApp(m_ElementData);
    m_TagStarted = false;
}

/**
 *	Callback when handling the value of an element
 */
void CBuildConfigParser::HandleCharacterData(const QString &data, int /*inLen*/)
{
    if (m_TagStarted)
        m_ElementData += data;
}

/**
 *	Read in all the project related attributes
 */
void CBuildConfigParser::ParseProjectAttributes(const QXmlStreamAttributes &inAttributes)
{
    for (const QXmlStreamAttribute &attrib : inAttributes) {
        if (attrib.name() == STR_PARAM_NAME)
            m_CurrentConfiguration->SetName(attrib.value().toUtf8().constData());
    }
}

/**
 *	Read in all the property related attributes
 */
void CBuildConfigParser::ParsePropertyAttributes(CBuildConfiguration::SConfigProperty &inProperty,
                                                 const QXmlStreamAttributes &inAttributes)
{
    for (const QXmlStreamAttribute &attrib : inAttributes) {
        if (attrib.name() == STR_PARAM_NAME)
            inProperty.SetName(attrib.value().toUtf8().constData());
        else if (attrib.name() == STR_PARAM_LABEL)
            inProperty.SetLabel(attrib.value().toUtf8().constData());
        else if (attrib.name() == STR_PARAM_HELP)
            inProperty.SetHelp(attrib.value().toUtf8().constData());
    }
}

/**
 *	Read in all the property value related attributes
 */
void CBuildConfigParser::ParseValueAttributes(const QXmlStreamAttributes &inAttributes)
{
    CBuildConfiguration::SConfigPropertyValue theValue;
    for (const QXmlStreamAttribute &attrib : inAttributes) {
        if (attrib.name() == STR_PARAM_NAME)
            theValue.SetName(attrib.value().toString());
        else if (attrib.name() == STR_PARAM_LABEL)
            theValue.SetLabel(attrib.value().toString());
    }

    if (m_CurrentProperty->GetName() == QLatin1String("MODE")
        && theValue.GetName() == QLatin1String("Debug")) {
        return;
    }

    m_CurrentProperty->AddValue(theValue);
}

} // namespace Q3DStudio
