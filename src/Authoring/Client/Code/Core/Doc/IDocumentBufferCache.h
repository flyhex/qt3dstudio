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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_IDOCUMENTBUFFERCACHE_H
#define INCLUDED_IDOCUMENTBUFFERCACHE_H
#include "Qt3DSFileTools.h"
#include "StudioObjectTypes.h"
#include "QS3DImageTextureData.h"
#include "QS3DRenderMesh.h"

#include <QtCore/qfileinfo.h>

class CDoc;

namespace Q3DStudio {

struct SModelBufferAndPath
{
    QS3DRenderMesh *m_modelBuffer;
    QFileInfo m_filePath;

    SModelBufferAndPath(QS3DRenderMesh *inBuffer, const QFileInfo &inPath)
        : m_modelBuffer(inBuffer)
        , m_filePath(inPath)
    {
    }

    SModelBufferAndPath()
        : m_modelBuffer(NULL)
    {
    }

    operator QS3DRenderMesh *() { return m_modelBuffer; }
    QS3DRenderMesh *operator->() { return m_modelBuffer; }
};

class IDocumentBufferCache
{
protected:
    virtual ~IDocumentBufferCache() {}
public:
    friend class std::shared_ptr<IDocumentBufferCache>;
    virtual const wchar_t **GetPrimitiveNames() = 0;
    virtual const wchar_t *GetPrimitiveName(EPrimitiveType inPrimitiveType) = 0;
    // Get or create the model buffer.  May return NULL if the sourcepath doesn't
    // map to a loadable model buffer.
    // Takes a *relative* path from the document
    virtual SModelBufferAndPath GetOrCreateModelBuffer(const QFileInfo &inSourcePath) = 0;
    //
    // Get or create the image buffer.  May return NULL if the sourcepath doesn't
    // map to a loadable image buffer.
    // Takes a *relative* path from the document
    virtual QS3DImageTextureData GetOrCreateImageBuffer(const QFileInfo &inSourcePath) = 0;

    virtual void
    GetImageBuffers(std::vector<std::pair<QString, QS3DImageTextureData>> &outBuffers) = 0;

    // Takes a *relative* path from the document
    virtual void InvalidateBuffer(const QFileInfo &inSourcePath) = 0;

    // Don't send events but just clear everything out.  Used on document::close
    virtual void Clear() = 0;

    static std::shared_ptr<IDocumentBufferCache> CreateBufferCache(CDoc &inDoc);
};
}

#endif
