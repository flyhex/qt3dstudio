/****************************************************************************
**
** Copyright (C) 1999-2005 Anark Corporation.
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

#include "Qt3DSCommonPrecompile.h"
#include "IDocumentBufferCache.h"
#include "Doc.h"
#include "foundation/Qt3DS.h"
#include "Qt3DSDMStringTable.h"
#include "EASTL/hash_map.h"

#include "foundation/StringTable.h"

#include "Core.h"
#include "Dispatch.h"
#include "IDocSceneGraph.h"
#include <unordered_map>

using namespace Q3DStudio;
using namespace qt3ds;
using namespace qt3ds::foundation;
using namespace std;
using qt3ds::foundation::IStringTable;
using qt3ds::foundation::CRegisteredString;

namespace {

using std::unordered_map;
typedef unordered_map<const wchar_t *, SRenderMesh *> TStringBufferMap;

struct SModelBufferOrImage
{
    // A file can contain multiple model buffers and we want to be accurate about reflecting this.
    TStringBufferMap m_ModelBuffers;
    Option<SImageTextureData> m_ImageBuffer;

    SModelBufferOrImage() {}
    SModelBufferOrImage(SImageTextureData &imageBuffer)
        : m_ImageBuffer(imageBuffer)
    {
    }
    void AddModelBuffer(const wchar_t *inBufferPath, SRenderMesh *inBuffer)
    {
        pair<TStringBufferMap::iterator, bool> theInsertResult(
            m_ModelBuffers.insert(make_pair(inBufferPath, inBuffer)));
        if (theInsertResult.second == false)
            theInsertResult.first->second = inBuffer;
    }
    SModelBufferAndPath FindModelBuffer(const wchar_t *inFullPath)
    {
        TStringBufferMap::iterator theBuffer = m_ModelBuffers.find(inFullPath);
        if (theBuffer != m_ModelBuffers.end())
            return SModelBufferAndPath(theBuffer->second, CFilePath(inFullPath));
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

    typedef unordered_map<const wchar_t *, SModelBufferOrImage> TBufferHashMap;

    qt3dsdm::TStringTablePtr m_StringTablePtr;
    TBufferHashMap m_Buffers;
    bool m_HasPrimitiveBuffers;

    SDocBufferCache(CDoc &inDoc)
        : m_Doc(inDoc)
        , m_StringTablePtr(qt3dsdm::IStringTable::CreateStringTable())
        , m_HasPrimitiveBuffers(false)
    {
    }

    ~SDocBufferCache() { m_Buffers.clear(); }

    IBufferManager *GetBufferManager()
    {
        if (m_Doc.GetSceneGraph()) {
            return m_Doc.GetSceneGraph()->GetBufferManager();
        }
        return NULL;
    }

    void Reset()
    {
        m_Buffers.clear();
        m_HasPrimitiveBuffers = false;
        if (GetBufferManager())
            GetBufferManager()->Clear();
    }

    void InvalidateBuffer(const CFilePath &inPath) override
    {
        CFilePath theFullPath = m_Doc.GetResolvedPathToDoc(inPath);
        if (GetBufferManager())
            GetBufferManager()->InvalidateBuffer(
                GetBufferManager()->GetStringTable().RegisterStr(theFullPath.toCString()));

        TBufferHashMap::iterator entry(
            m_Buffers.find(m_StringTablePtr->RegisterStr(inPath.toCString())));
        if (entry != m_Buffers.end()) {
            const_cast<SModelBufferOrImage &>(entry->second).Release();
            m_Buffers.erase(entry);
            m_Doc.GetCore()->GetDispatch()->FireDocumentBufferCacheInvalidated(inPath.toQString());
        }
    }

    // Names are declared in the same order as the primitives
    // offset by the empty primitive
    const wchar_t **GetPrimitiveNames() override
    {
        static const wchar_t *retval[] = {
            L"#Rectangle", L"#Sphere", L"#Cone", L"#Cylinder", L"#Cube", NULL,
        };
        return retval;
    }

    const wchar_t *GetPrimitiveName(EPrimitiveType inPrimitiveType) override
    {
        if (inPrimitiveType != PRIMITIVETYPE_UNKNOWN) {
            int theType(inPrimitiveType - 1);
            const wchar_t **theNames = GetPrimitiveNames();
            for (const wchar_t **theIter = theNames; theIter && *theIter; ++theIter, --theType) {
                if (theType == 0)
                    return *theIter;
            }
        }
        return L"";
    }

    void AddModelBuffer(const CFilePath inFile, SRenderMesh *inBuffer)
    {
        const wchar_t *thePath(m_StringTablePtr->RegisterStr(inFile.toCString()));
        TBufferHashMap::iterator theBufPtr =
            m_Buffers.insert(make_pair(thePath, SModelBufferOrImage())).first;
        const wchar_t *theFileWithId(m_StringTablePtr->RegisterStr(thePath));
        theBufPtr->second.AddModelBuffer(theFileWithId, inBuffer);
    }

    void BuildPrimitive(EPrimitiveType inPrimitiveType)
    {
        const wchar_t *thePrimitiveName(GetPrimitiveName(inPrimitiveType));
        if (GetBufferManager()) {
            SRenderMesh *theBuffer = GetBufferManager()->LoadMesh(
                GetBufferManager()->GetStringTable().RegisterStr(thePrimitiveName));
            AddModelBuffer(CFilePath(thePrimitiveName), theBuffer);
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

    SModelBufferAndPath GetOrCreateModelBuffer(const CFilePath &inPath) override
    {
        CheckAndCreatePrimitiveBuffers();

        CString path = inPath.toCString();
        const TBufferHashMap::iterator entry(
            m_Buffers.find(m_StringTablePtr->RegisterStr(path)));
        if (entry != m_Buffers.end()) {
            SModelBufferAndPath retval =
                entry->second.FindModelBuffer(m_StringTablePtr->RegisterStr(path));
            if (retval.m_ModelBuffer)
                return retval;
        }

        if (path.size() == 0 || path[0] == '#')
            return SModelBufferAndPath();

        CFilePath theFullPath(m_Doc.GetResolvedPathToDoc(path));
        SModelBufferAndPath retval;
        if (theFullPath.IsFile()) {
            if (GetBufferManager()) {
                SRenderMesh *theMesh = GetBufferManager()->LoadMesh(
                    GetBufferManager()->GetStringTable().RegisterStr(theFullPath.toCString()));
                if (theMesh) {
                    theFullPath.SetIdentifier(QString::number(theMesh->m_MeshId));
                    retval = SModelBufferAndPath(theMesh, theFullPath);
                }
            }
        }
        if (retval.m_ModelBuffer)
            AddModelBuffer(inPath, retval.m_ModelBuffer);
        return retval;
    }

    SImageTextureData GetOrCreateImageBuffer(const CFilePath &inPath) override
    {
        const TBufferHashMap::iterator entry(m_Buffers.find(m_StringTablePtr->RegisterStr(inPath.toCString())));
        if (entry != m_Buffers.end() && entry->second.m_ImageBuffer.hasValue())
            return entry->second.m_ImageBuffer;
        CFilePath thePath(m_Doc.GetResolvedPathToDoc(inPath));
        SImageTextureData retval;
        if (thePath.IsFile() && GetBufferManager()) {
            retval = GetBufferManager()->LoadRenderImage(
                GetBufferManager()->GetStringTable().RegisterStr(thePath.toCString()));
        }
        if (retval.m_Texture)
            m_Buffers.insert(
                make_pair(m_StringTablePtr->RegisterStr(inPath.toCString()), SModelBufferOrImage(retval)));
        return retval;
    }

    virtual void
    GetImageBuffers(std::vector<std::pair<Q3DStudio::CString, SImageTextureData>> &outBuffers) override
    {
        for (TBufferHashMap::iterator theIter = m_Buffers.begin(), theEnd = m_Buffers.end();
             theIter != theEnd; ++theIter) {
            if (theIter->second.m_ImageBuffer.hasValue())
                outBuffers.push_back(
                    make_pair(Q3DStudio::CString(theIter->first), theIter->second.m_ImageBuffer));
        }
    }

    void Clear() override { Reset(); }
};
}

std::shared_ptr<IDocumentBufferCache> IDocumentBufferCache::CreateBufferCache(CDoc &inDoc)
{
    return std::make_shared<SDocBufferCache>(std::ref(inDoc));
}
