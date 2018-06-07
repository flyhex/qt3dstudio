/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

#include "WGLRenderContext.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "render/Qt3DSRenderContext.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSLogging.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>
#include <QtWidgets/qopenglwidget.h>

//QT3DS_DEFINE_THISFILE;

//=============================================================================
/**
 *	Constructor:	Creates the object
 */
CWGLRenderContext::CWGLRenderContext(Qt3DSWindow inWindow)
    : m_qtContext(0)
    , m_Foundation(Q3DStudio::Foundation::SStudioFoundation::Create())
{
    Open(inWindow);
}

//=============================================================================
/**
 *	Destructor:	Destroys the object.
 */
CWGLRenderContext::~CWGLRenderContext()
{
    Close();
}

//=============================================================================
/**
 *	Open the render context.
 *	@param	inRenderWindow		window handle
 *	@param	inWindowSize		window size
 */
void CWGLRenderContext::Open(Qt3DSWindow inRenderWindow)
{
    // needed because NVidia cards will fail all the system functions below if there is no window.
    // note: the only time inRenderWindow is nullptr is when CThumbnailGenerator is used. Bug3075.
    if (!inRenderWindow)
        return;

    QObject* qObject = static_cast<QObject*>(inRenderWindow);
    QOpenGLWidget* qRenderWidget = qobject_cast<QOpenGLWidget*>(qObject);
    Q_ASSERT(qRenderWidget);

    OpenNormalContext(qRenderWidget);
}

static bool compareContextVersion(QSurfaceFormat a, QSurfaceFormat b)
{
    if (a.renderableType() != b.renderableType())
        return false;
    if (a.majorVersion() != b.majorVersion())
        return false;
    if (a.minorVersion() > b.minorVersion())
        return false;
    return true;
}

QSurfaceFormat CWGLRenderContext::selectSurfaceFormat(QOpenGLWidget* window)
{
    struct ContextVersion {
        int major;
        int minor;
        qt3ds::render::NVRenderContextType contextType;
    };

    ContextVersion versions[] = {
        {4, 5, qt3ds::render::NVRenderContextValues::GL4},
        {4, 4, qt3ds::render::NVRenderContextValues::GL4},
        {4, 3, qt3ds::render::NVRenderContextValues::GL4},
        {4, 2, qt3ds::render::NVRenderContextValues::GL4},
        {4, 1, qt3ds::render::NVRenderContextValues::GL4},
        {3, 3, qt3ds::render::NVRenderContextValues::GL3},
        {2, 1, qt3ds::render::NVRenderContextValues::GL2},
        {2, 0, qt3ds::render::NVRenderContextValues::GLES2},
        {0, 0, qt3ds::render::NVRenderContextValues::NullContext}
    };

    QSurfaceFormat result = window->format();
    bool valid = false;

    for (const auto& ver : versions) {
        if (ver.contextType == qt3ds::render::NVRenderContextValues::NullContext)
            break;

        // make an offscreen surface + context to query version
        QScopedPointer<QOffscreenSurface> offscreenSurface(new QOffscreenSurface);

        QSurfaceFormat format = window->format();
        if (ver.contextType == qt3ds::render::NVRenderContextValues::GLES2) {
            format.setRenderableType(QSurfaceFormat::OpenGLES);
        } else {
            format.setRenderableType(QSurfaceFormat::OpenGL);
            if (ver.major >= 2)
                format.setProfile(QSurfaceFormat::CoreProfile);
        }
        format.setMajorVersion(ver.major);
        format.setMinorVersion(ver.minor);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);

        offscreenSurface->setFormat(format);
        offscreenSurface->create();
        Q_ASSERT(offscreenSurface->isValid());

        QScopedPointer<QOpenGLContext> queryContext(new QOpenGLContext);
        queryContext->setFormat(format);
        if (queryContext->create() && compareContextVersion(format, queryContext->format())) {
            valid = true;
            result = format;
            break;
        }
    } // of version test iteration

    if (!valid) {
        qFatal("Unable to select suitable OpenGL context");
    }

    qDebug() << Q_FUNC_INFO << "selected surface format:" << result;
    QSurfaceFormat::setDefaultFormat(result);
    return result;
}

//=============================================================================
/**
 *	Open a non-multisample render context.
 *	@param	inRenderWindow		window handle
 *	@param	inWindowSize		window size
 *	@param	inPixelDesc			the pixel descriptor struct
 */
void CWGLRenderContext::OpenNormalContext(QOpenGLWidget* inRenderWindow)
{
    // Close before trying to open
    Close();

    // Save off the window
    m_Window = inRenderWindow;
    m_qtContext = m_Window->context();
    Q_ASSERT(m_qtContext);

    qt3ds::foundation::NVScopedRefCounted<qt3ds::foundation::IStringTable> theStringTable =
        qt3ds::foundation::IStringTable::CreateStringTable(*m_Foundation.m_AllocatorCallback);
    QSurfaceFormat contextFormat = m_qtContext->format();
    m_RenderContext = NVScopedRefCounted<NVRenderContext>(
        NVRenderContext::CreateGL(*m_Foundation.m_Foundation, *theStringTable,
                                  contextFormat));
    if (m_RenderContext) {
        m_RenderContext->SetDefaultDepthBufferBitCount(contextFormat.depthBufferSize());
        m_RenderContext->SetDefaultRenderTarget(inRenderWindow->defaultFramebufferObject());
    }
}

//=============================================================================
/**
 *	Close the render context.
 */
void CWGLRenderContext::Close()
{
    m_qtContext = 0;
}

//=============================================================================
/**
 *	Prepare the render context to begin rendering.
 */
void CWGLRenderContext::BeginRender()
{
    m_Window->makeCurrent();
    if (m_lastWidgetFBO != m_Window->defaultFramebufferObject()) {
        m_lastWidgetFBO = m_Window->defaultFramebufferObject();
        m_RenderContext->SetDefaultRenderTarget(m_lastWidgetFBO);
    }
}

//=============================================================================
/**
 *	Finalize the rendering of this frame, and present the result.
 */
void CWGLRenderContext::EndRender()
{
    m_Window->doneCurrent();
}

void CWGLRenderContext::resized()
{
    if (m_RenderContext) {
        m_RenderContext->SetDefaultRenderTarget(m_Window->defaultFramebufferObject());
    }
}

void CWGLRenderContext::requestRender()
{
    m_Window->update();
}
