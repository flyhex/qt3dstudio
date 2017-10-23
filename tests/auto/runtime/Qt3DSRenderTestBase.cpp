/****************************************************************************
**
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

#include "Qt3DSRenderTestBase.h"

using namespace qt3ds::render;

namespace qt3ds {
namespace render {

NVRenderTestBase::~NVRenderTestBase()
{
    delete m_renderImpl;
}

bool NVRenderTestBase::initializeQt3DSRenderer(QSurfaceFormat format)
{
    m_coreFactory = qt3ds::render::IQt3DSRenderFactoryCore::CreateRenderFactoryCore("", m_windowSystem,
                                                                                m_timeProvider);
    m_factory = m_coreFactory->CreateRenderFactory(format);
    m_rc = m_factory->GetQt3DSRenderContext();
    m_renderImpl = new qt3ds::render::Qt3DSRendererImpl(*m_rc);

    QString versionString;
    switch ((QT3DSU32)m_rc->GetRenderContext().GetRenderContextType()) {
    case NVRenderContextValues::GLES2:
        versionString = QLatin1Literal("gles2");
        break;
    case NVRenderContextValues::GL2:
        versionString = QLatin1Literal("gl2");
        break;
    case NVRenderContextValues::GLES3:
        versionString = QLatin1Literal("gles3");
        break;
    case NVRenderContextValues::GL3:
        versionString = QLatin1Literal("gl3");
        break;
    case NVRenderContextValues::GLES3PLUS:
        versionString = QLatin1Literal("gles3x");
        break;
    case NVRenderContextValues::GL4:
        versionString = QLatin1Literal("gl4");
        break;
    default:
        break;
    }

    m_rc->GetDynamicObjectSystem().setShaderCodeLibraryVersion(versionString);

    return true;
}

Qt3DSRendererImpl *NVRenderTestBase::qt3dsRenderer()
{
    return m_renderImpl;
}

Q3DStudio::IRuntimeMetaData *NVRenderTestBase::metadata()
{
    if (m_metaData.mPtr == NULL)
        m_metaData = &Q3DStudio::IRuntimeMetaData::Create(m_coreFactory->GetInputStreamFactory());
    return m_metaData.mPtr;
}

}
}
