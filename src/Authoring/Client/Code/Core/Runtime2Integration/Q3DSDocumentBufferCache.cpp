/****************************************************************************
**
** Copyright (C) 1999-2005 Anark Corporation.
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

#include "IDocumentBufferCache.h"
#include "Q3DSRenderBufferManager.h"
#include "Doc.h"
#include "Core.h"
#include "Dispatch.h"
#include "IDocSceneGraph.h"

#include <unordered_map>


using namespace Q3DStudio;

namespace {

typedef QHash<QString, Q3DSRenderMesh *> TStringBufferMap;

struct SModelBufferOrImage
{
    // A file can contain multiple model buffers and we want to be accurate about reflecting this.
    TStringBufferMap m_ModelBuffers;
    QSharedPointer<Q3DSImageTextureData> m_ImageBuffer;
    QHash<QString, QString> m_ModelBuffersidentifiers;

    SModelBufferOrImage() {}
    SModelBufferOrImage(Q3DSImageTextureData &imageBuffer)
        : m_ImageBuffer(&imageBuffer)
    {
    }
    void AddModelBuffer(const QString &inBufferPath, Q3DSRenderMesh *inBuffer,
                        const QString &identifier)
    {
        m_ModelBuffers.insert(inBufferPath, inBuffer);
        m_ModelBuffersidentifiers.insert(inBufferPath, identifier);
    }
    SModelBufferAndPath FindModelBuffer(const QString &inFullPath)
    {
        if (m_ModelBuffers.contains(inFullPath))
            return SModelBufferAndPath(m_ModelBuffers[inFullPath], QFileInfo(inFullPath),
                                       m_ModelBuffersidentifiers[inFullPath]);
        return SModelBufferAndPath();
    }
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

    Q3DSRenderBufferManager *GetBufferManager()
    {
        if (m_Doc.GetSceneGraph())
            return m_Doc.GetSceneGraph()->GetBufferManager();
        return nullptr;
    }

    void Reset()
    {
        m_Buffers.clear();
        m_HasPrimitiveBuffers = false;
        if (GetBufferManager())
            GetBufferManager()->Clear();
    }

    void InvalidateBuffer(const QFileInfo &inPath) override
    {
        QString theFullPath = m_Doc.GetResolvedPathToDoc(inPath);
        if (GetBufferManager())
            GetBufferManager()->InvalidateBuffer(theFullPath);

        const QString path = inPath.filePath();
        if (m_Buffers.contains(path)) {
            m_Buffers[path].Release();
            m_Buffers.remove(path);
            m_Doc.GetCore()->GetDispatch()
                    ->FireDocumentBufferCacheInvalidated(CString::fromQString(path));
        }
    }

    // Names are declared in the same order as the primitives
    // offset by the empty primitive
    QStringList GetPrimitiveNames() override
    {
        static const QStringList retval = {
            QStringLiteral("#Rectangle"), QStringLiteral("#Sphere"),
            QStringLiteral("#Cone"), QStringLiteral("#Cylinder"),
            QStringLiteral("#Cube")
        };
        return retval;
    }

    QString GetPrimitiveName(EPrimitiveType inPrimitiveType) override
    {
        if (inPrimitiveType != PRIMITIVETYPE_UNKNOWN) {
            int theType(inPrimitiveType - 1);
            if (GetPrimitiveNames().size() > theType)
                return GetPrimitiveNames()[theType];
        }
        return QStringLiteral("");
    }

    void AddModelBuffer(const QFileInfo inFile, Q3DSRenderMesh *inBuffer, const QString &identifier)
    {
        const QString path = inFile.filePath();
        m_Buffers.insert(path, SModelBufferOrImage());
        m_Buffers[path].AddModelBuffer(path, inBuffer, identifier);
    }

    void BuildPrimitive(EPrimitiveType inPrimitiveType)
    {
        QString thePrimitiveName(GetPrimitiveName(inPrimitiveType));
        if (GetBufferManager()) {
            Q3DSRenderMesh *theBuffer = GetBufferManager()->LoadMesh(thePrimitiveName);
            AddModelBuffer(CFilePath(thePrimitiveName), theBuffer, QString::number(0));
        }
    }

    void CheckAndCreatePrimitiveBuffers()
    {
        if (m_HasPrimitiveBuffers)
            return;
        m_HasPrimitiveBuffers = true;
        BuildPrimitive(PRIMITIVETYPE_BOX);
        BuildPrimitive(PRIMITIVETYPE_CONE);
        BuildPrimitive(PRIMITIVETYPE_CYLINDER);
        BuildPrimitive(PRIMITIVETYPE_RECT);
        BuildPrimitive(PRIMITIVETYPE_SPHERE);
    }

    SModelBufferAndPath GetOrCreateModelBuffer(const QFileInfo &inPath) override
    {
        CheckAndCreatePrimitiveBuffers();
        const QString path = inPath.filePath();

        if (m_Buffers.contains(path)) {
            SModelBufferAndPath retval = m_Buffers[path].FindModelBuffer(path);
            if (retval.m_modelBuffer)
                return retval;
        }

        if (path.size() == 0 || path[0] == '#')
            return SModelBufferAndPath();

        QString theFullPath(m_Doc.GetResolvedPathToDoc(inPath));
        SModelBufferAndPath retval;
        if (inPath.isFile()) {
            if (GetBufferManager()) {
                Q3DSRenderMesh *theMesh = GetBufferManager()->LoadMesh(theFullPath);
                if (theMesh) {
                    retval = SModelBufferAndPath(theMesh, theFullPath,
                                                 QString::number(theMesh->m_meshId));
                }
            }
        }
        if (retval.m_modelBuffer)
            AddModelBuffer(inPath, retval.m_modelBuffer, retval.m_identifier);
        return retval;
    }

    Q3DSImageTextureData GetOrCreateImageBuffer(const QFileInfo &inPath) override
    {
        const QString path = inPath.filePath();
        if (m_Buffers.contains(path)) {
            if (!m_Buffers[path].m_ImageBuffer.isNull())
                return *m_Buffers[path].m_ImageBuffer.data();
        }
        Q3DSImageTextureData retval;
        QString rpath = m_Doc.GetResolvedPathToDoc(inPath);
        if (inPath.isFile() && GetBufferManager())
            retval = GetBufferManager()->LoadRenderImage(rpath);

        if (retval.m_texture)
            m_Buffers.insert(path, SModelBufferOrImage(retval));

        return retval;
    }

    virtual void
    GetImageBuffers(std::vector<std::pair<QString, Q3DSImageTextureData>> &outBuffers) override
    {
        for (TBufferHashMap::iterator theIter = m_Buffers.begin(), theEnd = m_Buffers.end();
             theIter != theEnd; ++theIter) {
            if (!theIter->m_ImageBuffer.isNull()) {
                outBuffers.push_back(
                    std::make_pair(theIter.key(), *theIter.value().m_ImageBuffer));
            }
        }
    }

    void Clear() override { Reset(); }
};
}

std::shared_ptr<IDocumentBufferCache> IDocumentBufferCache::CreateBufferCache(CDoc &inDoc)
{
    return std::make_shared<SDocBufferCache>(std::ref(inDoc));
}
