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
#ifndef LENDIANSTREAMSH
#define LENDIANSTREAMSH
#include "IOStreams.h"

class CLEndianInStream : public CInStream
{
public:
    CLEndianInStream();
    CLEndianInStream(CInputStream *inStream);
    CLEndianInStream(const CInStream &inStream);
    virtual ~CLEndianInStream();

    CInStream *Copy(CInputStream *inBase) override;
    CInStream *Copy() override;

    CInStream &operator>>(double &outValue) override;
    CInStream &operator>>(float &outValue) override;
    CInStream &operator>>(long &outValue) override;
    CInStream &operator>>(unsigned long &outValue) override;
    CInStream &operator>>(short &outValue) override;
    CInStream &operator>>(wchar_t &outValue) override;
};

class CLEndianOutStream : public COutStream
{
public:
    CLEndianOutStream();
    CLEndianOutStream(COutputStream *inStream);
    virtual ~CLEndianOutStream();

    COutStream *Copy(COutputStream *inBase) override;
    COutStream *Copy() override;

    COutStream &operator<<(double inValue) override;
    COutStream &operator<<(float inValue) override;
    COutStream &operator<<(long inValue) override;
    COutStream &operator<<(unsigned long inValue) override;
    COutStream &operator<<(short inValue) override;
    COutStream &operator<<(wchar_t inValue) override;
};

#endif
