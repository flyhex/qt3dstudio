/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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
#ifndef QT3DS_RENDER_BUFFER_LOADED_H
#define QT3DS_RENDER_BUFFER_LOADED_H

#include "Qt3DSRender.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"

namespace qt3ds {
namespace render {

    class IBufferLoaderCallback;

    class ILoadedBuffer : public NVRefCounted
    {
    public:
        virtual CRegisteredString Path() = 0;
        // Data is released when the buffer itself is released.
        virtual NVDataRef<QT3DSU8> Data() = 0;
        virtual IBufferLoaderCallback *UserData() = 0;
    };

    class IBufferLoaderCallback : public NVRefCounted
    {
    public:
        virtual void OnBufferLoaded(ILoadedBuffer &inBuffer) = 0;
        virtual void OnBufferLoadFailed(CRegisteredString inPath) = 0;
        virtual void OnBufferLoadCancelled(CRegisteredString inPath) = 0;
    };

    // Job of this object is to load buffers all the way to memory as fast as possible.
    class IBufferLoader : public NVRefCounted
    {
    public:
        // nonblocking.  Quiet failure is passed to the input stream factory.
        // Returns handle to loading buffer
        virtual QT3DSU64 QueueForLoading(CRegisteredString inPath,
                                      IBufferLoaderCallback *inUserData = NULL,
                                      bool inQuietFailure = false) = 0;
        // Cancel a buffer that has not made it to the loaded buffers list.
        virtual void CancelBufferLoad(QT3DSU64 inBufferId) = 0;
        // If we were will to wait, will we ever get another buffer
        virtual bool WillLoadedBuffersBeAvailable() = 0;
        // Will nextLoadedBuffer block or not?
        virtual bool AreLoadedBuffersAvailable() = 0;

        // blocking, be careful with this.  No guarantees about timely return here.
        virtual NVScopedRefCounted<ILoadedBuffer> NextLoadedBuffer() = 0;

        static IBufferLoader &Create(NVFoundationBase &fnd, IInputStreamFactory &inFactory,
                                     IThreadPool &inThreadPool);
    };
}
}

#endif
