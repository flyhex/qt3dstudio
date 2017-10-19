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

#ifndef INCLUDED_FORMATTED_INPUT_STREAM_H
#define INCLUDED_FORMATTED_INPUT_STREAM_H 1

#pragma once

#include "Qt3DSFile.h"
#include "Qt3DSString.h"
#include "InputStream.h"

class CFormattedInputStream : public CInputStream
{
public:
    CFormattedInputStream(CInputStream *inInputStream);
    virtual ~CFormattedInputStream();

    long Read(void *inData, long inLength) override;
    void Close() override;
    bool IsValid() override;
    Q3DStudio::CString GetMimeType() override { return L""; }
    Q3DStudio::CString GetSource() override { return L""; }

    void SetBigEndian(bool inIsBigEndian);

    CFormattedInputStream &operator>>(long &);
    CFormattedInputStream &operator>>(unsigned long &);
    CFormattedInputStream &operator>>(float &);
    CFormattedInputStream &operator>>(short &);
    CFormattedInputStream &operator>>(Q3DStudio::UICChar &);
    CFormattedInputStream &operator>>(bool &);

    unsigned char ReadUnsignedChar();
    unsigned short ReadUnsignedShort();

    void CopyToFile(const CUICFile &inFilename, long inLength, bool inCloseStream = true);

protected:
    CInputStream *m_InputStream;
    bool m_IsBigEndian;
    long m_Count;
};
#endif // INCLUDED_FORMATTED_INPUT_STREAM_H
