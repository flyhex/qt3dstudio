/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "FileIterator.h"
#include "UICFile.h"

#include <QDirIterator>

using namespace Q3DStudio;

//=============================================================================
/**
 * Constructor: Takes a snapshot of the directory specified and all further
 * iterations refer to this snapshot.
 * @param inFile directory to iterate through
 */
CFileIterator::CFileIterator(const CUICFile *inFile)
{
    m_File = inFile->GetAbsolutePath();

    QDirIterator it(m_File.toQString(), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        it.next();
        QFileInfo theFindFile = it.fileInfo();
        Q3DStudio::CString theFullPath = Q3DStudio::CString::fromQString(QDir::toNativeSeparators(theFindFile.absoluteFilePath()));
        m_FileNames.push_back(theFullPath);
     }

    m_Index = 0;
}

//=============================================================================
/**
 * Destructor
 */
CFileIterator::~CFileIterator()
{
    m_FileNames.clear();
}

//=============================================================================
/**
 * @return true if iteration has completed, otherwise false
 */
bool CFileIterator::IsDone()
{
    return (m_Index >= m_FileNames.size());
}

//=============================================================================
/**
 * Increments the file iterator forward to the next file.
 */
void CFileIterator::operator++()
{
    ++m_Index;
}

//=============================================================================
/**
 * Increments the file iterator forward by the specified amount.
 * @param inNumToInc the number of files/directories to skip over
 */
void CFileIterator::operator+=(const long inNumToInc)
{
    m_Index += inNumToInc;
    if (m_Index > m_FileNames.size())
        m_Index = m_FileNames.size();
}

//=============================================================================
/**
 * @return the file or directory currently pointed to by this iterator
 */
CUICFile CFileIterator::GetCurrent()
{
    return CUICFile(m_FileNames[m_Index]);
}

//=============================================================================
/**
 * @return the number of files and directories in this iterator.
 */
size_t CFileIterator::GetTotal()
{
    return m_FileNames.size();
}
