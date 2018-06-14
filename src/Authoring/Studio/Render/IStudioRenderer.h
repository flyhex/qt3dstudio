/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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
#ifndef QT3DS_STUDIO_RENDERER
#define QT3DS_STUDIO_RENDERER
#pragma once

#include "IDocSceneGraph.h"
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include "Doc.h"

#include <QRect>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Q3DStudio {
using qt3ds::QT3DSI32;
class IStudioRenderer : public IDocSceneGraph
{
protected:
    virtual ~IStudioRenderer() {}

public:
    friend class std::shared_ptr<IStudioRenderer>;

    virtual bool IsInitialized() = 0;

    virtual void Initialize(QWidget *inWindow) = 0;

    virtual void SetViewRect(const QRect &inRect) = 0;

    virtual void GetEditCameraList(QStringList &outCameras) = 0;
    virtual void SetPolygonFillModeEnabled(bool inEnableFillMode) = 0;
    virtual bool IsPolygonFillModeEnabled() const = 0;
    virtual bool DoesEditCameraSupportRotation(QT3DSI32 inIndex) = 0;
    virtual bool AreGuidesEnabled() const = 0;
    virtual void SetGuidesEnabled(bool val) = 0;
    virtual bool AreGuidesEditable() const = 0;
    virtual void SetGuidesEditable(bool val) = 0;
    // Setting the camera to -1 disables the edit cameras
    // So setting the camera to 0- (numcameras - 1) will set change the active
    // edit camera.
    virtual void SetEditCamera(QT3DSI32 inIndex) = 0;
    virtual QT3DSI32 GetEditCamera() const = 0;
    virtual void EditCameraZoomToFit() = 0;

    // This must be safe to call from multiple places
    virtual void Close() = 0;

    // synchronously render the content
    virtual void RenderNow() = 0;

    virtual void MakeContextCurrent() = 0;
    virtual void ReleaseContext() = 0;

    virtual void RegisterSubpresentations(
            const QVector<SubPresentationRecord> &subpresentations) = 0;

    // Uses the global studio app to get the doc and dispatch.
    static std::shared_ptr<IStudioRenderer> CreateStudioRenderer();
};
};

#endif
