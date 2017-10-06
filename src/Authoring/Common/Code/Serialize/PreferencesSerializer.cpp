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
#include "stdafx.h"
#include "PreferencesSerializer.h"

#include <QDir>

CPreferencesSerializer::CPreferencesSerializer()
    : m_FileSet(false)
    , m_PreferencesStrTable(IStringTable::CreateStringTable())
    , m_PreferencesFactory(IDOMFactory::CreateDOMFactory(m_PreferencesStrTable))
{
}

CPreferencesSerializer::~CPreferencesSerializer()
{
    Serialize();
}

void CPreferencesSerializer::SetPreferencesFile(const Q3DStudio::CString &theFile)
{
    Q3DStudio::CFilePath fPath = Q3DStudio::CFilePath::Normalize(theFile);

    if (m_PreferencesFile.Compare(fPath, false))
        return;

    Serialize();

    m_PreferencesFile = fPath;

    SDOMElement *topElement = NULL;
    if (m_PreferencesFile.IsFile()) {
        qt3ds::foundation::CFileSeekableIOStream theInStream(m_PreferencesFile,
                                                             qt3ds::foundation::FileReadFlags());
        if (theInStream.IsOpen() == false) {
            QT3DS_ASSERT(false);
            m_FileSet = false;
            return;
        }

        topElement = CDOMSerializer::Read(*m_PreferencesFactory, theInStream);
        if (topElement == NULL) {
            QT3DS_ASSERT(false);
            m_FileSet = false;
            return;
        }
    } else {
        topElement = m_PreferencesFactory->NextElement(L"Settings");
    }

    m_PreferencesIO =
        IDOMWriter::CreateDOMWriter(m_PreferencesFactory, *topElement, m_PreferencesStrTable);
    m_FileSet = true;
}

void CPreferencesSerializer::Begin(const Q3DStudio::CString &inTag)
{
    if (!m_FileSet) {
        return;
    }
    if (!inTag.IsEmpty()) {
        std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
        CStringTokenizer theTokenizer(inTag, L"\\");
        do {
            Q3DStudio::CString theTag = theTokenizer.GetCurrentPartition();
            if (!theReader->MoveToFirstChild(theTag.c_str())) {
                std::shared_ptr<IDOMWriter> theWriter(m_PreferencesIO.first);
                theWriter->Begin(theTag.c_str());
            }
            ++theTokenizer;
        } while (theTokenizer.HasNextPartition());
    }
}

bool CPreferencesSerializer::GetSubElemValue(const Q3DStudio::CString &inSubElem,
                                             Q3DStudio::CString &outValue)
{
    if (!m_FileSet) {
        return false;
    }

    std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
    IDOMReader::Scope _readerScoped(*theReader);
    bool hasNext = true;

    for (hasNext = theReader->MoveToFirstChild(); hasNext;
         hasNext = theReader->MoveToNextSibling()) {
        TWideXMLCharPtr strValue;
        theReader->Att(L"name", strValue);
        if (inSubElem.CompareNoCase(strValue)) {
            theReader->Att(L"value", strValue);
            outValue.assign(strValue);
            return true;
        }
    }

    return false;
}

void CPreferencesSerializer::SetSubElemValue(const Q3DStudio::CString &inSubElem,
                                             const Q3DStudio::CString &inValue)
{
    if (!m_FileSet) {
        return;
    }

    std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
    theReader->PushScope();
    bool hasNext = true, hasFind = false;
    TWideXMLCharPtr strValue;
    for (hasNext = theReader->MoveToFirstChild(); hasNext;
         hasNext = theReader->MoveToNextSibling()) {

        theReader->Att(L"name", strValue);
        if (inSubElem.CompareNoCase(strValue)) {
            hasFind = true;
            break;
        }
    }
    if (hasFind) {
        std::shared_ptr<IDOMWriter> theWriter(m_PreferencesIO.first);
        theWriter->Att(L"value", inValue.c_str());
        theReader->PopScope();
    } else {
        theReader->PopScope();
        std::shared_ptr<IDOMWriter> theWriter(m_PreferencesIO.first);
        theWriter->Begin(L"Item");
        theWriter->Att(L"Name", inSubElem.c_str());
        theWriter->Att(L"value", inValue.c_str());
    }
}
void CPreferencesSerializer::Revert()
{
    if (!m_FileSet) {
        return;
    }

    std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
    theReader->SetScope(theReader->GetTopElement());
}

void CPreferencesSerializer::Remove(const Q3DStudio::CString &inTag)
{
    if (!m_FileSet) {
        return;
    }
    if (!inTag.IsEmpty()) {
        std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
        CStringTokenizer theTokenizer(inTag, L"\\");
        Q3DStudio::CString theTag;
        bool bBreak = true;
        do {
            theTag = theTokenizer.GetCurrentPartition();
            bBreak = theReader->MoveToFirstChild(theTag.c_str());
            if (!bBreak)
                break;
            ++theTokenizer;
        } while (theTokenizer.HasNextPartition());
        if (bBreak) {
            std::shared_ptr<IDOMWriter> theWriter(m_PreferencesIO.first);
            theWriter->RemoveCurrent();
        }
    } else {
        std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
        while (theReader->CountChildren() > 0) {
            theReader->MoveToFirstChild();
            std::shared_ptr<IDOMWriter> theWriter(m_PreferencesIO.first);
            theWriter->RemoveCurrent();
        }
    }
}

long CPreferencesSerializer::CountSubElems() const
{
    if (!m_FileSet) {
        return 0;
    }
    std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
    return static_cast<long>(theReader->CountChildren());
}

void CPreferencesSerializer::RemoveSubElem(const Q3DStudio::CString &inSubElem)
{
    if (!m_FileSet) {
        return;
    }
    std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
    IDOMReader::Scope _readerScoped(*theReader);
    bool hasNext = true;
    TWideXMLCharPtr strValue;
    for (hasNext = theReader->MoveToFirstChild(); hasNext;
         hasNext = theReader->MoveToNextSibling()) {
        theReader->Att(L"name", strValue);
        if (inSubElem.CompareNoCase(strValue)) {
            std::shared_ptr<IDOMWriter> theWriter(m_PreferencesIO.first);
            theWriter->RemoveCurrent();
            break;
        }
    }
}

bool CPreferencesSerializer::ExistElem(const Q3DStudio::CString &inElemName)
{
    if (!m_FileSet) {
        return false;
    }
    std::shared_ptr<IDOMReader> theReader(m_PreferencesIO.second);
    IDOMReader::Scope _readerScoped(*theReader);
    return theReader->MoveToFirstChild(inElemName.c_str());
}

void CPreferencesSerializer::Serialize()
{
    if (m_FileSet) {
        QString preferencesDir = m_PreferencesFile.GetDirectory().toQString();
        QDir dir(preferencesDir);
        if (!dir.exists()) {
            dir.mkpath(preferencesDir);
        }

        // Serialize the preferences in to the XML file
        qt3ds::foundation::CFileSeekableIOStream stream(m_PreferencesFile,
                                                        qt3ds::foundation::FileWriteFlags());
        stream.SetPosition(0, qt3ds::foundation::SeekPosition::Begin);
        CDOMSerializer::WriteXMLHeader(stream);
        CDOMSerializer::Write(*m_PreferencesIO.first->GetTopElement(), stream);
    }
}
