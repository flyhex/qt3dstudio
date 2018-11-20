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

#ifndef Q3DSDOCUMENTBUFFERCACHE_H
#define Q3DSDOCUMENTBUFFERCACHE_H

#include "IDocumentBufferCache.h"
#include "Q3DSRenderBufferManager.h"
#include "Doc.h"
#include "Core.h"
#include "Dispatch.h"
#include "IDocSceneGraph.h"

#include <unordered_map>

namespace Q3DStudio {

typedef QHash<QString, Q3DSRenderMesh *> TStringBufferMap;

struct SModelBufferOrImage
{
    // A file can contain multiple model buffers and we want to be accurate about reflecting this.
    TStringBufferMap m_ModelBuffers;
    Q3DSImageTextureData m_ImageBuffer;
    QHash<QString, QString> m_ModelBuffersidentifiers;

    SModelBufferOrImage() {}
    SModelBufferOrImage(const Q3DSImageTextureData &imageBuffer)
        : m_ImageBuffer(imageBuffer)
    {
    }
    void AddModelBuffer(const QString &inBufferPath, Q3DSRenderMesh *inBuffer,
                        const QString &identifier);
    SModelBufferAndPath FindModelBuffer(const QString &inFullPath);
    void Release() {}
};

struct SDocBufferCache : public IDocumentBufferCache
{
    CDoc &m_Doc;
    // This object cannot at this time have a ref count to the
    // buffer manager because in some cases will be destroyed after the gl context
    // is shut down thus causing chaos when the vertex buffers are released.

    typedef QHash<QString, SModelBufferOrImage> TBufferHashMap;

    TBufferHashMap m_Buffers;
    bool m_HasPrimitiveBuffers;

    SDocBufferCache(CDoc &inDoc)
        : m_Doc(inDoc)
        , m_HasPrimitiveBuffers(false)
    {
    }

    ~SDocBufferCache() override { m_Buffers.clear(); }

    Q3DSRenderBufferManager *GetBufferManager();
    void Reset();
    void InvalidateBuffer(const QFileInfo &inPath) override;

    // Names are declared in the same order as the primitives
    // offset by the empty primitive
    QStringList GetPrimitiveNames() override;
    QString GetPrimitiveName(EPrimitiveType inPrimitiveType) override;
    void AddModelBuffer(const QFileInfo &inFile, Q3DSRenderMesh *inBuffer,
                        const QString &identifier);
    void BuildPrimitive(EPrimitiveType inPrimitiveType);
    void CheckAndCreatePrimitiveBuffers();
    SModelBufferAndPath GetOrCreateModelBuffer(const QString &inPath) override;
    Q3DSImageTextureData GetOrCreateImageBuffer(const QString &inPath) override;
    virtual void
    GetImageBuffers(std::vector<std::pair<QString, Q3DSImageTextureData>> &outBuffers) override;
    void Clear() override { Reset(); }
};

}

#endif
