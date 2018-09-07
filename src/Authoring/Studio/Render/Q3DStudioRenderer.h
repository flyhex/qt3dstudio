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

#ifndef Q3DS_STUDIO_RENDERER_H
#define Q3DS_STUDIO_RENDERER_H

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

#include "IStudioRenderer.h"

namespace Q3DStudio {

class Q3DStudioRenderer : public IStudioRenderer
{
public:
    ITextRenderer *GetTextRenderer() override;
    QT3DSVec3 GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inHandle, CPt inPoint) override;
    Q3DSRenderBufferManager *GetBufferManager() override;
    IPathManager *GetPathManager() override;
    qt3ds::foundation::IStringTable *GetRenderStringTable() override;
    void RequestRender() override;
    bool IsInitialized() override;
    void Initialize(QWidget *inWindow)override;
    void SetViewRect(const QRect &inRect) override;
    void GetEditCameraList(QStringList &outCameras) override;
    void SetPolygonFillModeEnabled(bool inEnableFillMode) override;
    bool IsPolygonFillModeEnabled() const override;
    bool DoesEditCameraSupportRotation(QT3DSI32 inIndex) override;
    bool AreGuidesEnabled() const override;
    void SetGuidesEnabled(bool val) override;
    bool AreGuidesEditable() const override;
    void SetGuidesEditable(bool val) override;
    void SetEditCamera(QT3DSI32 inIndex) override;
    QT3DSI32 GetEditCamera() const override;
    void EditCameraZoomToFit() override;
    void Close() override;
    void RenderNow() override;
    void MakeContextCurrent() override;
    void ReleaseContext() override;
    void RegisterSubpresentations(
            const QVector<SubPresentationRecord> &subpresentations) override;
};

}

#endif
