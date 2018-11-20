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
                                                 Q3DSUipPresentation *presentation,
                                                 IInputStreamFactory &inInputStreamFactory)
    : m_engine(engine), m_presentation(presentation), m_inputStreamFactory(inInputStreamFactory)
{

}

QString Q3DSRenderBufferManager::GetImagePath(const QString &inSourcePath)
{
    return m_presentation->assetFileName(inSourcePath, nullptr);
}

Q3DSImageTextureData Q3DSRenderBufferManager::LoadRenderImage(const QString &inSourcePath,
                                                              bool inForceScanForTransparency,
                                                              bool inBsdfMipmaps)
{
    Q3DSImageTextureData data;
    QImage imageData(inSourcePath);
    if (imageData.isNull())
        return data;

    Q3DSUipPresentation *presentation = m_presentation;
    const QByteArray uniqueId(QByteArrayLiteral("Q3DSRBM::imageId"));
    Q3DSImage *image = presentation->newObject<Q3DSImage>(uniqueId);
    image->setSourcePath(inSourcePath);

    const bool transparency = image->hasTransparency(presentation);
    const bool premultiplied = image->hasPremultipliedAlpha();
    presentation->registerImageBuffer(inSourcePath, transparency);

    presentation->unlinkObject(image);
    delete image;

    data.m_width = imageData.width();
    data.m_height = imageData.width();
    data.m_valid = true;
    data.m_hasTransparency = transparency;
    data.m_premultiplied = premultiplied;
    return data;
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
    Q_ASSERT(m_presentation);
    m_presentation->registerImageBuffer(inSourcePath, inHasTransparency);
}

QSharedPointer<Q3DSRenderBufferManager>
Q3DSRenderBufferManager::Create(Q3DSEngine *engine, Q3DSUipPresentation *presentation,
                                IInputStreamFactory &inInputStreamFactory)
{
    static QSharedPointer<Q3DSRenderBufferManager> s_renderBufferManager;
    if (s_renderBufferManager.isNull() || s_renderBufferManager->m_engine != engine) {
        s_renderBufferManager.reset(new Q3DSRenderBufferManager(engine, presentation,
                                                                inInputStreamFactory));
    }
    return s_renderBufferManager;
}

}
