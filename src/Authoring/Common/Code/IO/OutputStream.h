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

#ifndef INCLUDED_OUTPUT_STREAM_H
#define INCLUDED_OUTPUT_STREAM_H 1
#include "Seekable.h"

// we probably want to change this architecture to match CInputStream which no longer has a seekable
// version - AES 12/26/02

class COutputStream
{
public:
    virtual ~COutputStream() {}
    virtual long Write(const void *inBuffer, long inBufferLength) = 0;
    virtual void Flush() = 0;
    virtual void Close() = 0;
    virtual bool IsValid() = 0;
};

class CSeekOutputStream : public COutputStream, public Q3DStudio::ISeekable
{
public:
    virtual ~CSeekOutputStream() {}
};
#endif // INCLUDED_OUTPUT_STREAM_H