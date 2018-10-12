/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
#include "FileOutputStream.h"
#include "IOLibraryException.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

CFileOutputStream::CFileOutputStream(const QString &inFilename, bool inAppend /* = false */)
    : m_Position(0)
    , m_Length(0)
{
#ifdef WIN32
    m_File = ::CreateFile(Q3DStudio::CString::fromQString(inFilename),
                          GENERIC_WRITE, FILE_SHARE_READ, NULL,
                          (inAppend) ? OPEN_ALWAYS : CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (m_File == INVALID_HANDLE_VALUE)
        throw CIOException();
    // In this case, set the file pointer to the end of the file
    if (inAppend)
        ::SetFilePointer(m_File, 0, NULL, FILE_END);
#else
    QByteArray theUTFBuffer = inFilename.toUtf8();
    m_File = ::fopen(theUTFBuffer.constData(), (inAppend) ? "a" : "w");
    if (m_File == NULL)
        throw CIOException();
#endif
}

CFileOutputStream::~CFileOutputStream()
{
    this->Close();
}

/** Write the specified buffer to this file.
 * @param inBuffer the bytes to write to this file.
 * @param inBufferLength the number of bytes to write.
 * @param outWroteCount the number of bytes actually written.
 */
long CFileOutputStream::Write(const void *inBuffer, long inBufferLength)
{
    long theWroteCount = 0;
#ifdef WIN32
    ::WriteFile(m_File, inBuffer, inBufferLength, (DWORD *)&theWroteCount, NULL);
    if (theWroteCount <= 0 && inBufferLength > 0)
        throw CIOException();
#else
    theWroteCount = ::fwrite(inBuffer, 1, inBufferLength, m_File);
    if (theWroteCount != inBufferLength)
        throw CIOException();
#endif
    m_Position += theWroteCount;
    if (m_Position > m_Length)
        m_Length = m_Position;

    return theWroteCount;
}

void CFileOutputStream::Flush()
{
#ifdef WIN32
#else
    ::fflush(m_File);
#endif
}

/** Closes the file.
 * This method should be called to eliminate handle leaks.
 */
void CFileOutputStream::Close()
{
#ifdef WIN32
    if (m_File != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_File);
        m_File = INVALID_HANDLE_VALUE;
    }
#else
    if (m_File != NULL) {
        ::fflush(m_File);
        ::fclose(m_File);
        m_File = NULL;
    }
#endif
}

/** Check to see if thus stream is valid (opened successfully and still open).
 */
bool CFileOutputStream::IsValid() const
{
#ifdef WIN32
    return (m_File != INVALID_HANDLE_VALUE);
#else
    return (m_File != NULL);
#endif
}

long CFileOutputStream::GetCurrentPosition()
{
    return m_Position;
}
