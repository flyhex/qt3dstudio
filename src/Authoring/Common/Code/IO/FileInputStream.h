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

//==============================================================================
//	Prefix
//==============================================================================

#ifndef INCLUDED_FILE_INPUT_STREAM_H
#define INCLUDED_FILE_INPUT_STREAM_H

#include <stdio.h>
#include "InputStream.h"
#include "UICString.h"

#include <QFile>

//==============================================================================
/**
 *	@class	CFileInputStream
 *	@brief	Brief description goes here.
 *
 *	This is where a complete desciption of the class belongs.
 */
class CFileInputStream : public CInputStream
{
protected:
    static const long INTERNAL_BUFFER_SIZE =
        4096; ///< Size of internal buffer that is reading chunks from file

    // Field Members
protected:
    Q3DStudio::CString m_FileName;

    char m_InternalBuffer[INTERNAL_BUFFER_SIZE];
    long m_ReadBytesFromCurrentBuffer;
    long m_AvailableBytesInBuffer;
    QFile m_File;

    // Construction
public:
    CFileInputStream(const Q3DStudio::CString &inFileName);
    virtual ~CFileInputStream();

    // CInputStream
public:
    long Seek(Q3DStudio::ISeekable::ESeekPosition inSeekPosition, long inOffset) override;
    long Read(void *inBuffer, long inBufferLength) override;
    void Close() override;
    bool IsValid() override;
    Q3DStudio::CString GetMimeType() override { return L""; }
    Q3DStudio::CString GetSource() override;
};

#endif // INCLUDED_FILEINPUTSTREAM_H
