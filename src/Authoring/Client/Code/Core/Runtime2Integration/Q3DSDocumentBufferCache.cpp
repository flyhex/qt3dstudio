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

#include "Q3DSDocumentBufferCache.h"

#include <unordered_map>

namespace Q3DStudio {

void SModelBufferOrImage::AddModelBuffer(const QString &inBufferPath, Q3DSRenderMesh *inBuffer,
                                         const QString &identifier)
{
    m_ModelBuffers.insert(inBufferPath, inBuffer);
    m_ModelBuffersidentifiers.insert(inBufferPath, identifier);
}

SModelBufferAndPath SModelBufferOrImage::FindModelBuffer(const QString &inFullPath)
{
    if (m_ModelBuffers.contains(inFullPath)) {
        return SModelBufferAndPath(m_ModelBuffers[inFullPath], QFileInfo(inFullPath),
                                   m_ModelBuffersidentifiers[inFullPath]);
    }
    return SModelBufferAndPath();
}


Q3DSRenderBufferManager *SDocBufferCache::GetBufferManager()
{
    if (m_Doc.GetSceneGraph())
        return m_Doc.GetSceneGraph()->GetBufferManager();
    return nullptr;
}

void SDocBufferCache::Reset()
{
    m_Buffers.clear();
    m_HasPrimitiveBuffers = false;
    if (GetBufferManager())
        GetBufferManager()->Clear();
}

void SDocBufferCache::InvalidateBuffer(const QFileInfo &inPath)
{
    QString theFullPath = m_Doc.GetResolvedPathToDoc(inPath);
    if (GetBufferManager())
        GetBufferManager()->InvalidateBuffer(theFullPath);

    const QString path = inPath.filePath();
    if (m_Buffers.contains(path)) {
        m_Buffers[path].Release();
        m_Buffers.remove(path);
        m_Doc.GetCore()->GetDispatch()->FireDocumentBufferCacheInvalidated(path);
    }
}

// Names are declared in the same order as the primitives
// offset by the empty primitive
QStringList SDocBufferCache::GetPrimitiveNames()
{
    static const QStringList retval = {
        QStringLiteral("#Rectangle"), QStringLiteral("#Sphere"),
        QStringLiteral("#Cone"), QStringLiteral("#Cylinder"),
        QStringLiteral("#Cube")
    };
    return retval;
}

QString SDocBufferCache::GetPrimitiveName(EPrimitiveType inPrimitiveType)
{
    if (inPrimitiveType != PRIMITIVETYPE_UNKNOWN) {
        int theType(inPrimitiveType - 1);
        if (GetPrimitiveNames().size() > theType)
            return GetPrimitiveNames()[theType];
    }
    return {};
}

void SDocBufferCache::AddModelBuffer(const QString &inFile, Q3DSRenderMesh *inBuffer,
                                     const QString &identifier)
{
    m_Buffers.insert(inFile, SModelBufferOrImage());
    m_Buffers[inFile].AddModelBuffer(inFile, inBuffer, identifier);
}

void SDocBufferCache::BuildPrimitive(EPrimitiveType inPrimitiveType)
{
    QString thePrimitiveName(GetPrimitiveName(inPrimitiveType));
    if (GetBufferManager()) {
        Q3DSRenderMesh *theBuffer = GetBufferManager()->LoadMesh(thePrimitiveName);
        AddModelBuffer(thePrimitiveName, theBuffer, QString::number(1));
    }
}

void SDocBufferCache::CheckAndCreatePrimitiveBuffers()
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

SModelBufferAndPath SDocBufferCache::GetOrCreateModelBuffer(const QString &inPath)
{
    CheckAndCreatePrimitiveBuffers();

    if (m_Buffers.contains(inPath)) {
        SModelBufferAndPath retval = m_Buffers[inPath].FindModelBuffer(inPath);
        if (retval.m_modelBuffer)
            return retval;
    }

    if (inPath.isEmpty() || inPath.startsWith(QLatin1Char('#')))
        return SModelBufferAndPath();

    QString theFullPath(m_Doc.GetResolvedPathToDoc(QFileInfo(inPath)));
    SModelBufferAndPath retval;
    QFileInfo rp(theFullPath);
    if (rp.isFile() && GetBufferManager()) {
        Q3DSRenderMesh *theMesh = GetBufferManager()->LoadMesh(theFullPath);
        if (theMesh) {
            retval = SModelBufferAndPath(theMesh, theFullPath,
                                         QString::number(theMesh->m_meshId));
        }
    }
    if (retval.m_modelBuffer)
        AddModelBuffer(inPath, retval.m_modelBuffer, retval.m_identifier);
    return retval;
}

Q3DSImageTextureData SDocBufferCache::GetOrCreateImageBuffer(const QString &inPath)
{
    if (m_Buffers.contains(inPath) && m_Buffers[inPath].m_ImageBuffer.m_valid)
        return m_Buffers[inPath].m_ImageBuffer;

    Q3DSImageTextureData retval;
    QString rpath = m_Doc.GetResolvedPathToDoc(QFileInfo(inPath));
    QFileInfo rp(rpath);
    if (rp.isFile() && GetBufferManager())
        retval = GetBufferManager()->LoadRenderImage(rpath);

    if (retval.m_valid)
        m_Buffers.insert(inPath, SModelBufferOrImage(retval));

    return retval;
}

void
SDocBufferCache::GetImageBuffers(std::vector<std::pair<QString, Q3DSImageTextureData>> &outBuffers)
{
    for (TBufferHashMap::iterator theIter = m_Buffers.begin(), theEnd = m_Buffers.end();
         theIter != theEnd; ++theIter) {
        if (theIter->m_ImageBuffer.m_valid)
            outBuffers.push_back(std::make_pair(theIter.key(), theIter.value().m_ImageBuffer));
    }
}

std::shared_ptr<IDocumentBufferCache> IDocumentBufferCache::CreateBufferCache(CDoc &inDoc)
{
    return std::make_shared<SDocBufferCache>(std::ref(inDoc));
}

}
