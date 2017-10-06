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
#ifndef MEMBUFH
#define MEMBUFH

namespace Q3DStudio {
/**
        @class CMemBuf
        @brief Memory buffer class to encapsulate the handling of memory.
*/

class CMemBuf
{
public:
    CMemBuf();
    virtual ~CMemBuf();
    virtual long GetSize();
    virtual unsigned char *GetBuffer();
    virtual unsigned char *Detach();
    virtual void SetDirty(long theDirty);
    virtual long GetDirty();
    virtual void SetSize(long theSize) = 0;

protected:
    unsigned char *m_Buffer;
    long m_BufSize;
    long m_Dirty;
};

// used for dynamic memory, will delete in some cases
/**
        @class CDynamicMemBuf
        @brief Encapsulates dealing with dynamic memory.
*/
class CDynamicMemBuf : public CMemBuf
{
public:
    CDynamicMemBuf();
    virtual ~CDynamicMemBuf();
    virtual void SetMem(unsigned char *theMemPtr, long theSize);
    void SetSize(long theSize) override;
    virtual void DisableMinimumBuffer(bool inFlag);

protected:
    static const long MIN_SIZE = 65535;
    bool m_DisableMinFlag;
};

// used for static memory that does not need to be deleted
/**
        @class CStaticMemBuf
        @brief Encapsulates dealing with static memory.
*/
class CStaticMemBuf : public CMemBuf
{
public:
    CStaticMemBuf();
    virtual ~CStaticMemBuf();
    virtual void SetMem(unsigned char *theMemPtr, long theSize);
    void SetSize(long theSize) override;
};
}

#endif
