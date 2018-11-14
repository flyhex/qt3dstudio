/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "Q3DSRenderBufferManager.h"
#include "q3dsruntime2api_p.h"

#include <QtCore/qsharedpointer.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qfileinfo.h>

namespace Q3DStudio {

Q3DSRenderBufferManager::Q3DSRenderBufferManager(Q3DSEngine *engine,
                                                 IInputStreamFactory &inInputStreamFactory)
    : m_engine(engine), m_inputStreamFactory(inInputStreamFactory)
{

}

QString Q3DSRenderBufferManager::GetImagePath(const QString &inSourcePath)
{
    return m_engine->presentation()->assetFileName(inSourcePath, nullptr);
}

Q3DSImageTextureData Q3DSRenderBufferManager::LoadRenderImage(const QString &inSourcePath,
                                                              bool inForceScanForTransparency,
                                                              bool inBsdfMipmaps)
{
    return Q3DSImageTextureData();
}

Q3DSRenderMesh *Q3DSRenderBufferManager::LoadMesh(const QString &inSourcePath)
{
    return nullptr;
}

Q3DSRenderMesh *Q3DSRenderBufferManager::CreateMesh(const char *inSourcePath, void *inVertData,
                                   unsigned int inNumVerts, unsigned int inVertStride,
                                   unsigned int *inIndexData,
                                   unsigned int inIndexCount
                                   /*, qt3ds::NVBounds3 inBounds*/)
{
    return nullptr;
}

void Q3DSRenderBufferManager::Clear()
{

}

void Q3DSRenderBufferManager::InvalidateBuffer(const QString &inSourcePath)
{

}

void Q3DSRenderBufferManager::SetImageHasTransparency(const QString &inSourcePath,
                                                      bool inHasTransparency)
{
    m_engine->presentation()->registerImageBuffer(inSourcePath, inHasTransparency);
}

QSharedPointer<Q3DSRenderBufferManager> Q3DSRenderBufferManager::Create(Q3DSEngine *engine,
                                               IInputStreamFactory &inInputStreamFactory)
{
    static QSharedPointer<Q3DSRenderBufferManager> s_renderBufferManager;
    if (s_renderBufferManager.isNull() || s_renderBufferManager->m_engine != engine)
        s_renderBufferManager.reset(new Q3DSRenderBufferManager(engine, inInputStreamFactory));
    return s_renderBufferManager;
}

}
