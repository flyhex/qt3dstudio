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

#include "EnginePrefix.h"

#include "Qt3DSSyncPrimitive.h"
#include "Qt3DSThreadManager.h"
#include "NVImageScaler.h"

//==============================================================================
// OpenKode
//==============================================================================
#ifdef _WIN32
#pragma warning(push, 3)
#endif
#include "OpenKodeInclude.h" // TODO AH is this enough, or is it required around nv_main/nv_main.h also????
#ifdef _WIN32
#pragma warning(pop)
#endif

static const Q3DStudio::INT32 CONST_THREAD_SLEEP = 10;

//==============================================================================
//	OS specific functions that are declared and used in our code, that need to be
//	defined somewhere else that has OS specific knowledge.
//==============================================================================

//==============================================================================
/**
 *	Create a lock primitive
 */
void AppCreateLockPrimitive(Q3DStudio::TLockPrimitive *inPrimitive)
{
    Q3DStudio_ASSERT(inPrimitive);
    *inPrimitive = reinterpret_cast<Q3DStudio::TLockPrimitive *>(kdThreadMutexCreate(NULL));
}

//==============================================================================
/**
*	Destroy a lock primitive
*/
void AppDestroyLockPrimitive(Q3DStudio::TLockPrimitive *inPrimitive)
{
    Q3DStudio_ASSERT(inPrimitive);
    kdThreadMutexFree(reinterpret_cast<KDThreadMutex *>(*inPrimitive));
    *inPrimitive = NULL;
}

//==============================================================================
/**
*	Enter the thread sync point
*/
void AppBeginSync(Q3DStudio::TLockPrimitive inPrimitive)
{
    Q3DStudio_ASSERT(inPrimitive);
    kdThreadMutexLock(reinterpret_cast<KDThreadMutex *>(inPrimitive));
}

//==============================================================================
/**
*	Leave the thread sync point
*/
void AppEndSync(Q3DStudio::TLockPrimitive inPrimitive)
{
    Q3DStudio_ASSERT(inPrimitive);
    kdThreadMutexUnlock(reinterpret_cast<KDThreadMutex *>(inPrimitive));
}

//==============================================================================
/**
*	Create a thread and return an abstract ID (void*)
*/
Q3DStudio::TThreadID AppCreateThread(Q3DStudio::TThreadSpinFunction inFunc, void *inUserData)
{
    KDThread *theThread = kdThreadCreate(NULL, inFunc, inUserData);
    return static_cast<Q3DStudio::TThreadID>(theThread);
}

//==============================================================================
/**
*	Destroy the thread
*/
void AppDestroyThread(Q3DStudio::TThreadID inThreadID)
{
    void *ignored = NULL;
    kdThreadJoin(static_cast<KDThread *>(inThreadID), &ignored);
}

//==============================================================================
/**
*	Call a OS specific sleep function, to yield the cpu to other threads
*/
void AppSleepThread()
{
    Q3DStudio_sleepmillisec(CONST_THREAD_SLEEP);
}

static const Q3DStudio::INT32 CMAX_THUMBNAIL_SIZE = 148;
//==============================================================================
/**
 *	Find the correct thumbnail scaling
 */
inline static Q3DStudio::FLOAT FindThumbnailScale(const Q3DStudio::INT32 inMaxThumbnailSize,
                                                  Q3DStudio::INT32 inSourceWidth,
                                                  Q3DStudio::INT32 inSourceHeight)
{
    Q3DStudio::FLOAT theXScale = inMaxThumbnailSize / (Q3DStudio::FLOAT)inSourceWidth;
    Q3DStudio::FLOAT theYScale = inMaxThumbnailSize / (Q3DStudio::FLOAT)inSourceHeight;
    return theXScale < theYScale ? theXScale : theYScale;
}

inline static Q3DStudio::INT32 FourByteAlign(Q3DStudio::INT32 inSrc)
{
    return (inSrc + 3) & ~3;
}

namespace Q3DStudio {

inline static BOOL FileExists(const char *inPath)
{
    KDFile *theFile = kdFopen(inPath, "rb");
    if (theFile != NULL) {
        kdFclose(theFile);
        return true;
    }
    return false;
}

} // namespace Q3DStudio
