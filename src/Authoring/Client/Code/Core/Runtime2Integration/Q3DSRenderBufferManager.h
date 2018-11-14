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

#ifndef Q3DS_RENDER_BUFFER_MANAGER_H
#define Q3DS_RENDER_BUFFER_MANAGER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "Q3DSImageTextureData.h"
#include "Q3DSRenderMesh.h"
#include "Q3DSInputStreamFactory.h"

#include "q3dsruntime2api_p.h"

namespace Q3DStudio {

class Q3DSRenderBufferManager
{
public:
    Q3DSRenderBufferManager(Q3DSEngine *engine, IInputStreamFactory &inInputStreamFactory);
    virtual ~Q3DSRenderBufferManager() {}
    // Path manipulation used to get the final path form a base path plus relative extension
    /*virtual QString CombineBaseAndRelative(const char8_t *inBase,
                                                     const char8_t *inRelative) = 0;*/
    virtual void SetImageHasTransparency(const QString &inSourcePath,
                                         bool inHasTransparency);
//    virtual bool GetImageHasTransparency(const QString & inSourcePath) const = 0;
//    virtual void SetImageTransparencyToFalseIfNotSet(const QString & inSourcePath) = 0;
//    virtual void SetInvertImageUVCoords(const QString & inSourcePath,
//                                        bool inShouldInvertCoords) = 0;

    // Returns true if this image has been loaded into memory
    // This call is threadsafe.  Nothing else on this object is guaranteed to be.
//    virtual bool IsImageLoaded(const QString & inSourcePath) = 0;

    // Alias one image path with another image path. Optionally this object will ignore the
    // call if the source path is already loaded. Aliasing is currently used to
    // allow a default image to be shown in place of an image that is loading offline.
    // Returns true if the image was aliased, false otherwise.
//    virtual bool AliasImagePath(const QString & inSourcePath, const QString & inAliasPath,
//                                bool inIgnoreIfLoaded) = 0;
//    virtual void UnaliasImagePath(const QString & inSourcePath) = 0;

    // Returns the given source path unless the source path is aliased; in which case returns
    // the aliased path.
    virtual QString GetImagePath(const QString &inSourcePath);
    // Returns a texture and a boolean indicating if this texture has transparency in it or not.
    // Can't name this LoadImage because that gets mangled by windows to LoadImageA (uggh)
    // In some cases we need to only scan particular images for transparency.
    /*virtual Q3DSImageTextureData LoadRenderImage(const QString & inImagePath,
                                              SLoadedTexture &inTexture,
                                              bool inForceScanForTransparency = false,
                                              bool inBsdfMipmaps = false) = 0;*/
    virtual Q3DSImageTextureData LoadRenderImage(const QString & inSourcePath,
                                              bool inForceScanForTransparency = false,
                                              bool inBsdfMipmaps = false);
    virtual Q3DSRenderMesh *LoadMesh(const QString & inSourcePath);

    virtual Q3DSRenderMesh *CreateMesh(const char *inSourcePath, void *inVertData,
                                       unsigned int inNumVerts, unsigned int inVertStride,
                                       unsigned int *inIndexData,
                                       unsigned int inIndexCount
                                       /*, qt3ds::NVBounds3 inBounds*/);

    // Remove *all* buffers from the buffer manager;
    virtual void Clear();
    virtual void InvalidateBuffer(const QString &inSourcePath);

    static QSharedPointer<Q3DSRenderBufferManager> Create(Q3DSEngine *engine,
                                                    IInputStreamFactory &inInputStreamFactory);
private:
    Q3DSEngine *m_engine;
    IInputStreamFactory &m_inputStreamFactory;
};

} // namespace Q3DStudio

#endif
