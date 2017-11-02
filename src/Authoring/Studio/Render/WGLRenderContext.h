/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#ifndef __WGLRENDERCONTEXT_H_
#define __WGLRENDERCONTEXT_H_
#include "PlatformTypes.h"
#include "Q3DStudioNVFoundation.h"
#include "render/Qt3DSRenderBaseTypes.h"

#include <QSurfaceFormat>

QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
QT_FORWARD_DECLARE_CLASS(QOpenGLWidget)

namespace qt3ds {
class NVFoundation;
}

namespace qt3ds {
namespace render {
    class NVRenderContext;
    class CAllocator;
}
}

using qt3ds::NVFoundation;
using qt3ds::render::NVRenderContext;
using qt3ds::render::CAllocator;
using qt3ds::foundation::NVScopedRefCounted;

class GLogErrorString;

//==============================================================================
/**
 *	@class CWGLRenderContext: The OpenGL subclass of the CRenderContext class.
 */
class CWGLRenderContext
{
    // Field Members
protected:
    QOpenGLContext *m_qtContext;

    Q3DStudio::Foundation::SStudioFoundation m_Foundation;
    NVScopedRefCounted<NVRenderContext> m_RenderContext;
    QOpenGLWidget* m_Window;
    qt3ds::render::NVRenderContextType m_ContextType;

    quint32 m_lastWidgetFBO = 0;

    // Construction
public:
    CWGLRenderContext(Qt3DSWindow inRenderWindow);
    ~CWGLRenderContext();

    // Access
public:
    void BeginRender();
    void EndRender();

    // Only available after open.
    NVRenderContext &GetRenderContext() { return *m_RenderContext; }

    static QSurfaceFormat selectSurfaceFormat(QOpenGLWidget* window);

    void resized();

    void requestRender();

    // Implementation
protected:
    void Open(Qt3DSWindow inRenderWindow);

    void OpenNormalContext(QOpenGLWidget* inRenderWindow);
    void Close();
};

#endif // __WGLRENDERCONTEXT_H_
