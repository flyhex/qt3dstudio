/****************************************************************************
**
** Copyright (C) 2000 NVIDIA Corporation.
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

#ifndef INCLUDED_MUTEX_H
#define INCLUDED_MUTEX_H

#pragma once

#include "ThreadException.h"

#include <QMutex>

class CMutex
{
    typedef QMutex TMutex;

public:
    struct Scope
    {
    private:
        Scope(const Scope &inOther);
        Scope &operator=(const Scope &inOther);

    public:
        CMutex *m_Mutex;
        Scope(CMutex *inMutex)
            : m_Mutex(inMutex)
        {
            if (m_Mutex)
                m_Mutex->Lock();
        }
        ~Scope()
        {
            if (m_Mutex)
                m_Mutex->Unlock();
        }
    };
    CMutex();
    ~CMutex();

    void Lock();
    void Unlock();

    TMutex *GetMutex();

protected:
    TMutex m_Mutex;

private:
    CMutex(const CMutex &inCMutex);
    CMutex &operator=(const CMutex &inCMutex);
};

//=============================================================================
/**
 * Constructor for a new CMutex.
 */
inline CMutex::CMutex()
    : m_Mutex(QMutex::Recursive)
{
}

inline CMutex::~CMutex()
{
}

//=============================================================================
/**
 * Lock this mutex.
 */
inline void CMutex::Lock()
{
    m_Mutex.lock();
}

//=============================================================================
/**
 * Unlock this mutex.
 */
inline void CMutex::Unlock()
{
    m_Mutex.unlock();
}

//=============================================================================
/**
 * Get the underlying platform object this is operating on.
 */
inline CMutex::TMutex *CMutex::GetMutex()
{
    return &m_Mutex;
}
#endif // INCLUDED_MUTEX_H
