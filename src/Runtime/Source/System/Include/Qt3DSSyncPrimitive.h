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
#ifdef _MSC_VER
#pragma warning(push, 2)
#endif //_MSC_VER

#ifdef _PCPLATFORM
#include <windows.h>
#elif _XENONPLATFORM
#include <process.h> // which one we need??
#include <xtl.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER

//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {

typedef void *TLockPrimitive;

//==============================================================================
/**
 *	The CSyncPrimitive is a wrapper class to handle syncronization of data
 *	between threads.
 */
class CSyncPrimitive
{

    //==============================================================================
    //	Fields
    //==============================================================================
private:
    TLockPrimitive m_LockHandle;

    //==============================================================================
    //	Methods
    //==============================================================================
public:
    CSyncPrimitive();
    ~CSyncPrimitive();

public:
    int BeginSync();
    int EndSync();
};

//==============================================================================
/**
 *	Wrap the lock and unlocking of a sync primitive in an easy to use stack
 *	based class.
 */
class CSyncGuard
{
    //==============================================================================
    //	Fields
    //==============================================================================
private:
    CSyncPrimitive &m_Mutex;

    //==============================================================================
    //	Methods
    //==============================================================================
private:
    CSyncGuard(const CSyncGuard &other);
    CSyncGuard &operator=(const CSyncGuard &other);

public:
    CSyncGuard(CSyncPrimitive &inMutex)
        : m_Mutex(inMutex)
    {
        m_Mutex.BeginSync();
    }
    ~CSyncGuard() { m_Mutex.EndSync(); }
};

} // namespace Q3DStudio

//===================================================================
// The application needs to provide these thread primitive functions
//===================================================================

void AppCreateLockPrimitive(Q3DStudio::TLockPrimitive *inPrimitive);
void AppDestroyLockPrimitive(Q3DStudio::TLockPrimitive *inPrimitive);
void AppBeginSync(Q3DStudio::TLockPrimitive inPrimitive);
void AppEndSync(Q3DStudio::TLockPrimitive inPrimitive);