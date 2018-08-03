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

#include "Q3DStudioRenderer.h"

namespace Q3DStudio {

ITextRenderer *Q3DStudioRenderer::GetTextRenderer()
{
    return nullptr;
}

QT3DSVec3 Q3DStudioRenderer::GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inHandle, CPt inPoint)
{
    return QT3DSVec3();
}

Q3DSRenderBufferManager *Q3DStudioRenderer::GetBufferManager()
{
    return nullptr;
}

IPathManager *Q3DStudioRenderer::GetPathManager()
{
    return nullptr;
}

qt3ds::foundation::IStringTable *Q3DStudioRenderer::GetRenderStringTable()
{
    return nullptr;
}

void Q3DStudioRenderer::RequestRender()
{

}

bool Q3DStudioRenderer::IsInitialized()
{
    return false;
}

void Q3DStudioRenderer::Initialize(QWidget *inWindow)
{

}

void Q3DStudioRenderer::SetViewRect(const QRect &inRect)
{

}

void Q3DStudioRenderer::SetPolygonFillModeEnabled(bool inEnableFillMode)
{

}

bool Q3DStudioRenderer::IsPolygonFillModeEnabled() const
{
    return false;
}

void Q3DStudioRenderer::GetEditCameraList(QStringList &outCameras)
{

}

bool Q3DStudioRenderer::DoesEditCameraSupportRotation(QT3DSI32 inIndex)
{
    return false;
}

bool Q3DStudioRenderer::AreGuidesEnabled() const
{
    return false;
}

void Q3DStudioRenderer::SetGuidesEnabled(bool val)
{

}

bool Q3DStudioRenderer::AreGuidesEditable() const
{
    return false;
}

void Q3DStudioRenderer::SetGuidesEditable(bool val)
{

}

void Q3DStudioRenderer::SetEditCamera(QT3DSI32 inIndex)
{

}

QT3DSI32 Q3DStudioRenderer::GetEditCamera() const
{
    return 0;
}

void Q3DStudioRenderer::EditCameraZoomToFit()
{

}

void Q3DStudioRenderer::Close()
{

}

void Q3DStudioRenderer::RenderNow()
{

}

void Q3DStudioRenderer::MakeContextCurrent()
{

}

void Q3DStudioRenderer::ReleaseContext()
{

}

void Q3DStudioRenderer::RegisterSubpresentations(
        const QVector<SubPresentationRecord> &subpresentations){

}

std::shared_ptr<IStudioRenderer> IStudioRenderer::CreateStudioRenderer()
{
    return std::shared_ptr<IStudioRenderer>(new Q3DStudioRenderer());
}

}
