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

#include "stdafx.h"
#include "Strings.h"
#include "StringLoader.h"
#include <QtCore/qfile.h>
#include <QtCore/qxmlstream.h>

CStringLoader CStringLoader::s_GlobalInstance;

CStringLoader::CStringLoader()
    : m_Strings(nullptr)
    , m_StringCount(0)
{
}

CStringLoader::~CStringLoader()
{
    UnloadResourceStrings();
}

//=============================================================================
/**
 * Static function to load the string resource specified by inStringID.
 * inStringID should have been specified in the Resource.h header file.
 * This will load the string from the global string resource object.
 */
Q3DStudio::CString CStringLoader::LoadString(long inStringID)
{
    return s_GlobalInstance.LoadResourceString(inStringID);
}

//=============================================================================
/**
 * Load the string resource specified by inStringID.
 * inStringID should have been specified in the Resource.h header file.
 * @param inID the ID of the string to be loaded.
 * @return theString specified by inStringID.
 */
Q3DStudio::CString CStringLoader::LoadResourceString(long inStringID)
{
    Q3DStudio::CString theName;
    // Make sure we aren't going off into la-la land.
    if (inStringID > 0 && inStringID < m_StringCount) {
        theName = m_Strings[inStringID];
    }
    return theName;
}

//=============================================================================
/**
 * Load the string resources from the specified directory into the global table.
 * All .stro files in the directory will be processed for strings.
 * @param inDirectory the directory that should be read for strings.
 */
void CStringLoader::LoadStrings(const Qt3DSFile &inDirectory)
{
    s_GlobalInstance.LoadResourceStrings(inDirectory);
}

//=============================================================================
/**
 * Load the string resources from the specified directory into this string table.
 * All .stro files in the directory will be processed for strings.
 * @param inDirectory the directory that should be read for strings.
 */
void CStringLoader::LoadResourceStrings(const Qt3DSFile &inDirectory)
{
    UnloadResourceStrings();

    // Sure hope we don't get an absolute ton of strings here...
    m_Strings = new Q3DStudio::CString[STRING_RESOURCE_COUNT + 1];
    m_StringCount = STRING_RESOURCE_COUNT;

    Q3DStudio::CString theExtension = ".stro";

    // Go through all the files in the directory and process them.
    CFileIterator theFiles = inDirectory.GetSubItems();
    for (; !theFiles.IsDone(); ++theFiles) {
        Qt3DSFile theFile = theFiles.GetCurrent();
        Q3DStudio::CString theFilename = theFile.GetAbsolutePath();
        // Only process .stro files.
        if (theFilename.Find(theExtension) == theFilename.Length() - theExtension.Length()) {
            QFile file(theFilename.toQString());
            file.open(QFile::ReadOnly);
            QXmlStreamReader reader(&file);
            while (!reader.atEnd()) {
                reader.readNextStartElement();
                if (reader.name() == "string") {
                    Q3DStudio::CString theValue;
                    long theIndex = -1;
                    for (auto attrib : reader.attributes()) {
                        if (attrib.name() == "value")
                            theValue = attrib.value().toUtf8().constData();
                        else if (attrib.name() == "ID")
                            theIndex = attrib.value().toInt();
                    }
                    if (theIndex > 0 && theIndex < STRING_RESOURCE_COUNT) {
                        theValue.Replace("\\n", "\n");
                        theValue.Replace("\\t", "\t");
                        m_Strings[theIndex] = theValue;
                    }
                }
            }
        }
    }
}

void CStringLoader::UnloadStrings()
{
    s_GlobalInstance.UnloadResourceStrings();
}

void CStringLoader::UnloadResourceStrings()
{
    delete[] m_Strings;
    m_Strings = nullptr;
    m_StringCount = 0;
}

Q3DStudio::CString LoadResourceString(long inID)
{
    return CStringLoader::LoadString(inID);
}
