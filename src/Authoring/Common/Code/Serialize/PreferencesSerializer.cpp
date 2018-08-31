/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "PreferencesSerializer.h"
#include "Q3DSStringTable.h"

#include <QtCore/qdir.h>

CPreferencesSerializer::CPreferencesSerializer()
    : m_fileSet(false)
    , m_preferencesFactory(IDOMFactory::CreateDOMFactory(Q3DStudio::Q3DSStringTable::instance()))
{
}

CPreferencesSerializer::~CPreferencesSerializer()
{
    Serialize();
}

void CPreferencesSerializer::SetPreferencesFile(const QString &theFile)
{
    QFileInfo fPath(theFile);

    if (m_preferencesFile.filePath() == fPath.filePath())
        return;

    Serialize();

    m_preferencesFile = fPath;

    SDOMElement *topElement = nullptr;
    if (m_preferencesFile.isFile()) {
        QFile theInStream(fPath.filePath());
        if (theInStream.open(QFile::ReadOnly | QFile::Text | QFile::ExistingOnly) == false) {
            QT3DS_ASSERT(false);
            m_fileSet = false;
            return;
        }

        topElement = CDOMSerializer::Read(*m_preferencesFactory, theInStream);
        if (topElement == nullptr) {
            QT3DS_ASSERT(false);
            m_fileSet = false;
            return;
        }
    } else {
        topElement = m_preferencesFactory->NextElement(L"Settings");
    }

    m_preferencesIO =
        IDOMWriter::CreateDOMWriter(m_preferencesFactory, *topElement,
                                    Q3DStudio::Q3DSStringTable::instance());
    m_fileSet = true;
}

void CPreferencesSerializer::Begin(const QString &inTag)
{
    if (!m_fileSet)
        return;

    if (!inTag.isEmpty()) {
        std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
        const QStringList tokens = inTag.split(QStringLiteral("\\"));
        for (const QString &tag : tokens) {
            Q3DStudio::WQString token = Q3DStudio::toWQString(tag);
            if (!theReader->MoveToFirstChild(token.data())) {
                std::shared_ptr<IDOMWriter> theWriter(m_preferencesIO.first);
                theWriter->Begin(token.data());
            }
        }
    }
}

bool CPreferencesSerializer::GetSubElemValue(const QString &inSubElem,
                                             QString &outValue)
{
    if (!m_fileSet)
        return false;

    std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
    IDOMReader::Scope _readerScoped(*theReader);
    bool hasNext = true;

    for (hasNext = theReader->MoveToFirstChild(); hasNext;
         hasNext = theReader->MoveToNextSibling()) {
        TWideXMLCharPtr strValue;
        theReader->Att(L"name", strValue);
        QString value = QString::fromWCharArray(strValue);
        if (inSubElem.compare(value, Qt::CaseInsensitive) == 0) {
            theReader->Att(L"value", strValue);
            outValue = QString::fromWCharArray(strValue);
            return true;
        }
    }
    return false;
}

void CPreferencesSerializer::SetSubElemValue(const QString &inSubElem,
                                             const QString &inValue)
{
    if (!m_fileSet)
        return;

    std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
    theReader->PushScope();
    bool hasNext = true, hasFind = false;
    TWideXMLCharPtr strValue;
    for (hasNext = theReader->MoveToFirstChild(); hasNext;
         hasNext = theReader->MoveToNextSibling()) {

        theReader->Att(L"name", strValue);
        QString value = QString::fromWCharArray(strValue);
        if (inSubElem.compare(value, Qt::CaseInsensitive) == 0) {
            hasFind = true;
            break;
        }
    }
    if (hasFind) {
        std::shared_ptr<IDOMWriter> theWriter(m_preferencesIO.first);
        Q3DStudio::WQString iv = Q3DStudio::toWQString(inValue);
        theWriter->Att(L"value", iv.constData());
        theReader->PopScope();
    } else {
        theReader->PopScope();
        std::shared_ptr<IDOMWriter> theWriter(m_preferencesIO.first);
        theWriter->Begin(L"Item");
        Q3DStudio::WQString is = Q3DStudio::toWQString(inSubElem);
        Q3DStudio::WQString iv = Q3DStudio::toWQString(inValue);
        theWriter->Att(L"Name", is.constData());
        theWriter->Att(L"value", iv.constData());
    }
}
void CPreferencesSerializer::Revert()
{
    if (!m_fileSet)
        return;

    std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
    theReader->SetScope(theReader->GetTopElement());
}

void CPreferencesSerializer::Remove(const QString &inTag)
{
    if (!m_fileSet)
        return;

    if (!inTag.isEmpty()) {
        std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
        const QStringList tags = inTag.split(QStringLiteral("\\"));

        bool bBreak = true;
        for (const QString &tag : tags) {
            Q3DStudio::WQString wtag = Q3DStudio::toWQString(tag);
            bBreak = theReader->MoveToFirstChild(wtag.constData());
            if (!bBreak)
                break;
        };
        if (bBreak) {
            std::shared_ptr<IDOMWriter> theWriter(m_preferencesIO.first);
            theWriter->RemoveCurrent();
        }
    } else {
        std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
        while (theReader->CountChildren() > 0) {
            theReader->MoveToFirstChild();
            std::shared_ptr<IDOMWriter> theWriter(m_preferencesIO.first);
            theWriter->RemoveCurrent();
        }
    }
}

long CPreferencesSerializer::CountSubElems() const
{
    if (!m_fileSet)
        return 0;

    std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
    return static_cast<long>(theReader->CountChildren());
}

void CPreferencesSerializer::RemoveSubElem(const QString &inSubElem)
{
    if (!m_fileSet)
        return;

    std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
    IDOMReader::Scope _readerScoped(*theReader);
    bool hasNext = true;
    TWideXMLCharPtr strValue;
    for (hasNext = theReader->MoveToFirstChild(); hasNext;
         hasNext = theReader->MoveToNextSibling()) {
        theReader->Att(L"name", strValue);
        if (inSubElem.compare(QString::fromWCharArray(strValue), Qt::CaseInsensitive) == 0) {
            std::shared_ptr<IDOMWriter> theWriter(m_preferencesIO.first);
            theWriter->RemoveCurrent();
            break;
        }
    }
}

bool CPreferencesSerializer::ExistElem(const QString &inElemName)
{
    if (!m_fileSet)
        return false;

    std::shared_ptr<IDOMReader> theReader(m_preferencesIO.second);
    IDOMReader::Scope _readerScoped(*theReader);
    Q3DStudio::WQString name = Q3DStudio::toWQString(inElemName);
    return theReader->MoveToFirstChild(name.data());
}

void CPreferencesSerializer::Serialize()
{
    if (m_fileSet) {
        QString preferencesDir = m_preferencesFile.dir().path();
        QDir dir(preferencesDir);
        if (!dir.exists())
            dir.mkpath(preferencesDir);

        // Serialize the preferences in to the XML file
        QFile stream(m_preferencesFile.filePath());
        if (stream.open(QFile::Text | QFile::Truncate | QFile::WriteOnly)) {
            stream.seek(0);
            CDOMSerializer::WriteXMLHeader(stream);
            CDOMSerializer::Write(*m_preferencesIO.first->GetTopElement(), stream);
        }
    }
}
