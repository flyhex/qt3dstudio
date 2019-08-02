/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#ifndef QT3DS_STUDIO_HELPER_GRID_WIDGET_H
#define QT3DS_STUDIO_HELPER_GRID_WIDGET_H

#include "Qt3DSRender.h"
#include "Qt3DSRenderWidgets.h"
#include "Qt3DSRenderNode.h"
#include "foundation/Qt3DSContainers.h"

using namespace qt3ds::render;

namespace qt3ds {
namespace widgets {

struct SHelperGridWidget : public IRenderWidget, public NVRefCounted
{
    QT3DSVec3 m_gridColor = QT3DSVec3(0.5f, 0.5f, 0.5f);
    QT3DSVec3 m_xColor = QT3DSVec3(1.f, 0.f, 0.f);
    QT3DSVec3 m_yColor = QT3DSVec3(0.f, 1.f, 0.f);
    QVector<QT3DSVec3> m_lineData;
    int m_lineCount = 10; // Number of lines each side of center line
    float m_lineSpacing = 50;
    QT3DSMat44 m_rotation;
    NVScopedRefCounted<NVRenderInputAssembler> m_inputAssembler;
    NVScopedRefCounted<NVRenderShaderProgram> m_shader;
    CRegisteredString m_itemName;
    bool m_dirty;

    NVAllocatorCallback &m_allocator;
    volatile QT3DSI32 mRefCount;

    SHelperGridWidget(NVAllocatorCallback &inAlloc);
    virtual ~SHelperGridWidget() override {}

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_allocator)

    void setNode(SNode *node);
    void rotate(float angleRadians, const QT3DSVec3 &axis);
    void setColors(const QColor &gridColor, const QColor &xColor, const QColor &yColor);
    void setLines(int count, float spacing);
    void setupShader(IRenderWidgetContext &context);
    void setupGraphicsObjects(IRenderWidgetContext &context, NVDataRef<QT3DSVec3> lines);
    void Render(IRenderWidgetContext &widgetContext, NVRenderContext &renderContext) override;

    static SHelperGridWidget &createHelperGridWidget(NVAllocatorCallback &alloc);
};

}
}


#endif // QT3DS_STUDIO_HELPER_GRID_WIDGET_H
