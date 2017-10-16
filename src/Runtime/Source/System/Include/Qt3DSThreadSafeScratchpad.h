/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSSyncPrimitive.h"
#include "Qt3DSArray.h"
#include "foundation/Qt3DSLogging.h"

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

//==============================================================================
/**
 *	Definition of a scratch pad buffer
 */
struct SThreadScratchPadBuffer
{
    INT32 m_Size;
    INT32 m_RequestedSize;
    CHAR *m_Data;
};

//==============================================================================
/**
 *	This class wraps up a simple thread safe buffer manager class.  This
 *  has two functions that threaded
 */
class CThreadSafeScratchPad
{
    //==============================================================================
    //	Private Methods
    //==============================================================================
private:
    CThreadSafeScratchPad(const CThreadSafeScratchPad &other);
    CThreadSafeScratchPad &operator=(const CThreadSafeScratchPad &other);

    //==============================================================================
    //	Fields
    //==============================================================================
private:
    struct SBufferItem
    {
        SThreadScratchPadBuffer m_Buffer;
        BOOL m_InUse;
        BOOL m_Unused[3];
    };

    CArray<SBufferItem> m_Buffers;
    CSyncPrimitive m_Primitive;

    //==============================================================================
    //	Methods
    //==============================================================================
public:
    CThreadSafeScratchPad() {}
    ~CThreadSafeScratchPad();

    // Not a threadsafe call; used for testing purposes.
    INT32 GetBufferCount() const { return (INT32)m_Buffers.GetCount(); }
    SThreadScratchPadBuffer GetBuffer(INT32 inMinSize);
    void ReleaseBuffer(SThreadScratchPadBuffer inBuffer);
};
}

#include "Qt3DSThreadSafeScratchpad.inl"
