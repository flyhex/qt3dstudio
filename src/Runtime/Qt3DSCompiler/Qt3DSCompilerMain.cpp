/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "render/Qt3DSRenderContext.h"
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include "Qt3DSTypes.h"
#ifdef _WIN32
#pragma warning(pop)
#endif
#include "Qt3DSRenderRuntimeBinding.h"
#include "Qt3DSApplication.h"
#include "Qt3DSInputEngine.h"
#include "foundation/FileTools.h"
#include "Qt3DSWindowSystem.h"
#include "Qt3DSRenderContextCore.h"
#include "Qt3DSRenderShaderCache.h"

#include <QSurfaceFormat>

using namespace qt3ds::render;

#if defined(_LINUX) || defined(_MACOSX) || defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
namespace qt3ds {
void NVAssert(const char *exp, const char *file, int line, bool *igonore)
{
    qCritical() << "NVAssertion thrown: " << file << " " << line << " " << exp;
}
}

#ifndef EASTL_DEBUG_BREAK
void EASTL_DEBUG_BREAK()
{
    qFatal() << "EASTL_DEBUG_BREAK: Assertion blown";
}

#endif
#endif

struct SNullTimeProvider : public Q3DStudio::ITimeProvider
{
    Q3DStudio::INT64 GetCurrentTimeMicroSeconds() override { return 0; }
};

struct SNullWindowSystem : public Q3DStudio::IWindowSystem
{
    Q3DStudio::SSize GetWindowDimensions() override { return Q3DStudio::SSize(); }

    void SetWindowDimensions(const Q3DStudio::SSize &) override {}
    // For platforms that support it, we get the egl info for render plugins
    // Feel free to return NULL.
    Q3DStudio::SEGLInfo *GetEGLInfo() override { return NULL; }

    // on some systems we allow our default render target to be a offscreen buffer
    // otherwise return 0;
    int GetDefaultRenderTargetID() override { return 0; }
    // returns the depth buffer bit count for the render window
    // does not really matter here
    int GetDepthBitCount() override { return 16; }
};

int main(int c, char **v)
{
    QGuiApplication app(c, v);

    QSurfaceFormat format;
    format.setDepthBufferSize(32);
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QT_PREPEND_NAMESPACE(QOpenGLContext) *context = new QT_PREPEND_NAMESPACE(QOpenGLContext);
    context->setFormat(format);
    bool success = context->create();
    if (!success)
        qDebug() << "Could not create opengl context";

    QOffscreenSurface *surface = new QOffscreenSurface;
    surface->setFormat(format);
    surface->create();
    context->makeCurrent(surface);

    if (c < 2) {
        puts("Usage: UICCompiler project.uia\n"
             "No other usage is appropriate and there are at this time no options\n"
             "Binary files will be under projectdir/binary/\n"
             "Similar to if you had run UICViewer project.uia -saveBinary\n"
             "With the important difference that the shader cache is neither created nor updated");
        return -1;
    }
    using namespace qt3ds;
    using namespace qt3ds::foundation;

    eastl::string uiaFile(v[1]);

    eastl::string theApplicationDirectory
        = QCoreApplication::applicationDirPath().toLatin1().constData();
    SNullTimeProvider theNullTimeProvider;
    SNullWindowSystem theNullWindowSystem;

    NVScopedRefCounted<IQt3DSRenderFactoryCore> theRenderFactoryCore =
        IQt3DSRenderFactoryCore::CreateRenderFactoryCore(theApplicationDirectory.c_str(),
                                                       theNullWindowSystem, theNullTimeProvider);

    NVScopedRefCounted<IQt3DSRenderFactory> theRenderFactory(
        theRenderFactoryCore->CreateRenderFactory(QSurfaceFormat::defaultFormat()));

    // Ensure we aren't compiling any shaders.
    theRenderFactory->GetQt3DSRenderContext().GetShaderCache().SetShaderCompilationEnabled(false);
    Q3DStudio::CInputEngine theInputEngine;

    NVScopedRefCounted<qt3ds::runtime::IApplicationCore> theAppCore =
        qt3ds::runtime::IApplicationCore::CreateApplicationCore(*theRenderFactoryCore,
                                                              theApplicationDirectory.c_str());

    bool loadStarted = theAppCore->BeginLoad(uiaFile.c_str());
    if (loadStarted) {
        NVScopedRefCounted<qt3ds::runtime::IApplication> theApp =
            theAppCore->CreateApplication(theInputEngine, 0, *theRenderFactory);
        theApp->SaveBinary();
        return 0;
    } else {
        printf("Failed to compile %s\n, please review log output for reason\n", uiaFile.c_str());
        return -1;
    }
}
