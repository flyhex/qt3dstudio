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

#ifndef INCLUDED_GUARD_H
#define INCLUDED_GUARD_H

#pragma once

#include "Mutex.h"
class CGuard
{
public:
    CGuard(CMutex *inMutex, bool inLockNow = true);
    CGuard(CMutex &inMutex, bool inLockNow = true);
    ~CGuard();

    bool Lock();
    void Unlock();

private:
    CGuard();
    CMutex *m_Mutex;
    long m_LockCount;
};

//=============================================================================
/**
 * Constructor, simple Mutex Guard creation.
 * This will just lock the mutex on creation and unlock it on destruction.
 */
inline CGuard::CGuard(CMutex *inMutex, bool inLockNow)
    : m_LockCount(0)
{
    m_Mutex = inMutex;

    if (inLockNow)
        Lock();
}

inline CGuard::CGuard(CMutex &inMutex, bool inLockNow)
    : m_LockCount(0)
{
    m_Mutex = &inMutex;

    if (inLockNow)
        Lock();
}

inline CGuard::~CGuard()
{
    Unlock();
}

inline bool CGuard::Lock()
{

    m_Mutex->Lock();

    m_LockCount++;

    // Should return true if the lock was acquired
    return true;
}

inline void CGuard::Unlock()
{
    if (m_LockCount > 0) {
        m_LockCount--;
        m_Mutex->Unlock();
    }
}

#endif // INCLUDED_GUARD_H