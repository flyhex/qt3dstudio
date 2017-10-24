/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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

#ifndef QT3DS_QML_RENDER_H
#define QT3DS_QML_RENDER_H

#include "Qt3DSOffscreenRenderManager.h"
#include "Qt3DSRenderContextCore.h"

class IQt3DS;

using namespace qt3ds::render;

class IQ3DSQmlStreamService;
class IQ3DSQmlStreamRenderer;

class Q3DSQmlRender : public IOffscreenRenderer
{
public:
    Q3DSQmlRender(IQt3DSRenderContext &inRenderContext, const char *asset);
    ~Q3DSQmlRender();

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE_OVERRIDE(m_RenderContext.GetAllocator())

    CRegisteredString GetOffscreenRendererType() override;

    SOffscreenRendererEnvironment GetDesiredEnvironment(QT3DSVec2 inPresentationScaleFactor) override;

    // Returns true of this object needs to be rendered, false if this object is not dirty
    SOffscreenRenderFlags NeedsRender(const SOffscreenRendererEnvironment &inEnvironment,
                                              QT3DSVec2 inPresentationScaleFactor) override;

    void Render(const SOffscreenRendererEnvironment &inEnvironment,
                        NVRenderContext &inRenderContext, QT3DSVec2 inPresentationScaleFactor,
                        SScene::RenderClearCommand inColorBufferNeedsClear) override;

    IGraphObjectPickQuery *GetGraphObjectPickQuery() override
    {
        return nullptr;
    }
    bool Pick(const QT3DSVec2 &inMouseCoords, const QT3DSVec2 &inViewportDimensions) override
    {
        Q_UNUSED(inMouseCoords)
        Q_UNUSED(inViewportDimensions)
        return false;
    }
    static const char *GetRendererName() { return "qml-render"; }
private:

    void initializeRenderer();

    IQt3DSRenderContext &m_RenderContext;
    IQ3DSQmlStreamRenderer *m_qmlStreamRenderer;
    CRegisteredString m_offscreenRenderType;
    CRegisteredString m_assetString;
    volatile QT3DSI32 mRefCount;
};

#endif
