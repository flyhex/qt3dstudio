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

#ifndef INCLUDED_FILE_OUTPUT_STREAM_H
#define INCLUDED_FILE_OUTPUT_STREAM_H

#include "OutputStream.h"
#include "Qt3DSString.h"
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

//==============================================================================
/**
 *	@class	CFileOutputStream
 *	@brief	Brief description goes here.
 *	This is where a complete desciption of the class belongs.
 */
class CFileOutputStream : public CSeekOutputStream
{
protected:
#ifdef WIN32
    HANDLE m_File;
#else
    FILE *m_File;
#endif
    long m_Position; ///< Stores current write position
    long m_Length; ///< Stores length of data written to file so far

public:
    // Construction
    CFileOutputStream(const Q3DStudio::CString &inFilename, bool inAppend = false);
    virtual ~CFileOutputStream();

    long Write(const void *inBuffer, long inBufferLength) override;
    void Flush() override;
    void Close() override;
    bool IsValid() override;

    // CSeekOutputStream
    long GetCurrentPosition() override;
};

#endif // INCLUDED_FILE_OUTPUT_STREAM_H
