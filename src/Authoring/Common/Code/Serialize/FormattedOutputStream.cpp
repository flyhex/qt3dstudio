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

#include "Qt3DSCommonPrecompile.h"
#include "Qt3DSEndian.h"

#include "FormattedOutputStream.h"

CFormattedOutputStream::CFormattedOutputStream(CSeekOutputStream *inOutputStream)
    : m_OutputStream(inOutputStream)
{
}

CFormattedOutputStream::~CFormattedOutputStream()
{
}

long CFormattedOutputStream::Write(const void *inData, long inLength)
{
    return m_OutputStream->Write(inData, inLength);
}

void CFormattedOutputStream::Close()
{
    m_OutputStream->Close();
}

void CFormattedOutputStream::Flush()
{
    m_OutputStream->Flush();
}

bool CFormattedOutputStream::IsValid() const
{
    return m_OutputStream->IsValid();
}

CFormattedOutputStream &CFormattedOutputStream::operator<<(float inValue)
{
    m_OutputStream->Write(&inValue, sizeof(float));

    return *this;
}

CFormattedOutputStream &CFormattedOutputStream::operator<<(long inValue)
{
#ifdef WIN32
    m_OutputStream->Write(&inValue, sizeof(long));
#else
    SWAP4BYTES(inValue, inValue);
    m_OutputStream->Write(&inValue, sizeof(long));
#endif

    return *this;
}

CFormattedOutputStream &CFormattedOutputStream::operator<<(short inValue)
{
#ifdef WIN32
    m_OutputStream->Write(&inValue, sizeof(short));
#else
    SWAP2BYTES(inValue, inValue);
    m_OutputStream->Write(&inValue, sizeof(short));
#endif

    return *this;
}

CFormattedOutputStream &CFormattedOutputStream::operator<<(Q3DStudio::Qt3DSChar inValue)
{
#ifdef WIN32
    m_OutputStream->Write(&inValue, sizeof(Q3DStudio::Qt3DSChar));
#else
    SWAP2BYTES(inValue, inValue);
    m_OutputStream->Write(&inValue, sizeof(Q3DStudio::Qt3DSChar));
#endif

    return *this;
}

CFormattedOutputStream &CFormattedOutputStream::operator<<(bool inValue)
{
    m_OutputStream->Write(&inValue, sizeof(bool));

    return *this;
}
