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

#include <memory>

#include "Q3DStudioRenderer.h"
#include "StudioApp.h"
#include "StudioPreferences.h"
#include "StudioProjectSettings.h"
#include "Q3DSTranslation.h"

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qscreen.h>
#include <QtGui/qpainter.h>
#include <QtGui/qopenglpaintdevice.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>
#include <QtCore/qscopedvaluerollback.h>

#include <Qt3DRender/qrendersurfaceselector.h>
#include <Qt3DRender/private/qrendersurfaceselector_p.h>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qaspectengine.h>
#include <Qt3DRender/qrenderaspect.h>
#include <Qt3DRender/private/qrenderaspect_p.h>
#include <QtGui/private/qopenglcontext_p.h>

namespace Q3DStudio {

Q3DStudioRenderer::Q3DStudioRenderer()
    : m_dispatch(*g_StudioApp.GetCore()->GetDispatch())
    , m_doc(*g_StudioApp.GetCore()->GetDoc())
{
    m_dispatch.AddReloadListener(this);
    m_dispatch.AddDataModelListener(this);
    m_dispatch.AddPresentationChangeListener(this);
    m_selectionSignal
            = m_dispatch.ConnectSelectionChange(std::bind(&Q3DStudioRenderer::OnSelectionChange,
                                                          this));
    m_dispatch.AddSceneDragListener(this);
    m_dispatch.AddToolbarChangeListener(this);

    ::CColor color = CStudioPreferences::GetRulerBackgroundColor(); // Rectangles under tick marks
    m_rectColor = QColor(int(color.GetRed()), int(color.GetGreen()), int(color.GetBlue()));
    color = CStudioPreferences::GetRulerTickColor(); // Tick marks
    m_lineColor = QColor(int(color.GetRed()), int(color.GetGreen()), int(color.GetBlue()));
}

Q3DStudioRenderer::~Q3DStudioRenderer()
{
    OnClosingPresentation();
    m_dispatch.RemoveDataModelListener(this);
    m_dispatch.RemovePresentationChangeListener(this);
    m_dispatch.RemoveSceneDragListener(this);
    m_dispatch.RemoveToolbarChangeListener(this);
}

QSharedPointer<Q3DSEngine> &Q3DStudioRenderer::engine()
{
    return m_engine;
}

ITextRenderer *Q3DStudioRenderer::GetTextRenderer()
{
    return nullptr;
}

QT3DSVec3 Q3DStudioRenderer::GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inHandle,
                                                 CPt inPoint)
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
    if (m_widget && !m_renderRequested) {
        m_widget->update();
        m_renderRequested = true;
    }
}

bool Q3DStudioRenderer::IsInitialized()
{
    return !m_engine.isNull();
}

void Q3DStudioRenderer::Initialize(QWidget *inWindow)
{
    m_widget = qobject_cast<QOpenGLWidget *>(inWindow);
}

void Q3DStudioRenderer::SetViewRect(const QRect &inRect)
{
    m_viewRect = inRect;
    QSize size(inRect.width(), inRect.height());
    if (!m_engine.isNull())
        m_engine->resize(size, fixedDevicePixelRatio(), true);
    sendResizeToQt3D(size);
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
    return m_guidesEnabled;
}

void Q3DStudioRenderer::SetGuidesEnabled(bool val)
{
    m_guidesEnabled = val;
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

static void drawTopBottomTickMarks(QPainter &painter, qreal posX, qreal innerBottom,
                                   qreal innerTop, qreal outerBottom, qreal outerTop,
                                   qreal lineHeight)
{
    painter.drawLine(QLineF(posX, innerBottom - lineHeight, posX, innerBottom));
    painter.drawLine(QLineF(posX, innerTop, posX, innerTop + lineHeight));
}

static void drawLeftRightTickMarks(QPainter &painter, qreal inYPos, qreal innerLeft,
                                   qreal innerRight, qreal outerLeft, qreal outerRight,
                                   qreal lineLength)
{
    painter.drawLine(QLineF(innerLeft - lineLength, inYPos, innerLeft, inYPos));
    painter.drawLine(QLineF(innerRight, inYPos, innerRight + lineLength, inYPos));
}

void Q3DStudioRenderer::drawTickMarksOnHorizontalRects(QPainter &painter, qreal innerLeft,
                                                       qreal innerRight, qreal innerBottom,
                                                       qreal innerTop, qreal outerBottom,
                                                       qreal outerTop)
{
    qreal centerPosX = floor(innerLeft + (innerRight - innerLeft) / 2.0 + .5);
    drawTopBottomTickMarks(painter, centerPosX, innerBottom, innerTop, outerBottom,
                           outerTop, 15);
    for (unsigned int incrementor = 10;
         (centerPosX + incrementor) < innerRight && (centerPosX - incrementor) > innerLeft;
         incrementor += 10) {
        qreal rightEdge = centerPosX + incrementor;
        qreal leftEdge = centerPosX - incrementor;
        qreal lineHeight = 0;
        if (incrementor % 100 == 0)
            lineHeight = 11;
        else if (incrementor % 20)
            lineHeight = 4;
        else
            lineHeight = 2;

        if (rightEdge < innerRight) {
            drawTopBottomTickMarks(painter, rightEdge, innerBottom, innerTop, outerBottom,
                                   outerTop, lineHeight);
        }
        if (leftEdge > innerLeft) {
            drawTopBottomTickMarks(painter, leftEdge, innerBottom, innerTop, outerBottom,
                                   outerTop, lineHeight);
        }
    }
}

void Q3DStudioRenderer::drawTickMarksOnVerticalRects(QPainter &painter, qreal innerLeft,
                                                     qreal innerRight, qreal innerBottom,
                                                     qreal innerTop, qreal outerLeft,
                                                     qreal outerRight)
{
    qreal centerPosY = floor(innerBottom + (innerTop - innerBottom) / 2.0 + .5);
    drawLeftRightTickMarks(painter, centerPosY, innerLeft, innerRight, outerLeft,
                           outerRight, 15);
    for (unsigned int incrementor = 10;
         (centerPosY + incrementor) < innerTop && (centerPosY - incrementor) > innerBottom;
         incrementor += 10) {
        qreal topEdge = centerPosY + incrementor;
        qreal bottomEdge = centerPosY - incrementor;
        qreal lineHeight = 0;
        if (incrementor % 100 == 0)
            lineHeight = 11;
        else if (incrementor % 20)
            lineHeight = 4;
        else
            lineHeight = 2;

        if (topEdge < innerTop) {
            drawLeftRightTickMarks(painter, topEdge, innerLeft, innerRight, outerLeft,
                                   outerRight, lineHeight);
        }
        if (bottomEdge > innerBottom) {
            drawLeftRightTickMarks(painter, bottomEdge, innerLeft, innerRight, outerLeft,
                                   outerRight, lineHeight);
        }
    }
}

void Q3DStudioRenderer::drawGuides()
{
    if (!m_guidesEnabled)
        return;

    QOpenGLPaintDevice device;
    device.setSize(m_widget->size());
    QPainter painter(&device);

    QRect thePresentationViewport = m_viewRect;

    int offset = CStudioPreferences::guideSize() / 2;

    int innerLeft = thePresentationViewport.left() + offset;
    int innerRight = thePresentationViewport.right() - offset;
    int innerBottom = thePresentationViewport.bottom() - offset;
    int innerTop = thePresentationViewport.top() + offset;

    int outerLeft = innerLeft - offset;
    int outerRight = innerRight + offset;
    int outerBottom = innerBottom + offset;
    int outerTop = innerTop - offset;

    // Retain the rects for picking purposes.
    m_innerRect = QRect(innerLeft, innerTop, innerRight - innerLeft, innerBottom - innerTop);
    m_outerRect = QRect(outerLeft, outerTop, outerRight - outerLeft, outerBottom - outerTop);

    // Draw tick marks around the presentation
    painter.fillRect(QRect(outerLeft, innerTop,
                           innerLeft - outerLeft, innerBottom - innerTop),
                     m_rectColor);
    painter.fillRect(QRect(innerRight, innerTop,
                           outerRight - innerRight, innerBottom - innerTop),
                     m_rectColor);
    painter.fillRect(QRect(innerLeft, innerBottom,
                           innerRight - innerLeft, outerBottom - innerBottom),
                     m_rectColor);
    painter.fillRect(QRect(innerLeft, outerTop,
                           innerRight - innerLeft, innerTop - outerTop),
                     m_rectColor);

    painter.setPen(m_lineColor);
    drawTickMarksOnHorizontalRects(painter, innerLeft, innerRight, innerTop, innerBottom,
                                   outerTop, outerBottom);

    drawTickMarksOnVerticalRects(painter, innerLeft, innerRight, innerTop, innerBottom,
                                 outerLeft, outerRight);
}

void Q3DStudioRenderer::RenderNow()
{
    m_renderRequested = false;
    QOpenGLContextPrivate *ctxD = QOpenGLContextPrivate::get(m_widget->context());
    QScopedValueRollback<GLuint> defaultFboRedirectRollback(ctxD->defaultFboRedirect, 0);

    if (m_engine.isNull()) {
        createEngine();

        auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                    Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        renderAspectD->renderInitialize(m_widget->context());
    }

    m_widget->makeCurrent();

    if (!m_translation.isNull())
        m_translation->render();

    auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
    renderAspectD->renderSynchronous();

    drawGuides();
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

void Q3DStudioRenderer::OnBeginDataModelNotifications()
{

}

void Q3DStudioRenderer::OnEndDataModelNotifications()
{

}

void Q3DStudioRenderer::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{

}

void Q3DStudioRenderer::OnImmediateRefreshInstanceMultiple(
        qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{

}

void Q3DStudioRenderer::OnReloadEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{

}

void Q3DStudioRenderer::OnNewPresentation()
{
    m_hasPresentation = true;
    createTranslation();
}

void Q3DStudioRenderer::OnClosingPresentation()
{
    if (!m_engine.isNull())
        m_engine->setPresentation(nullptr);
    m_translation.reset();
    m_hasPresentation = false;
}

void Q3DStudioRenderer::OnSceneMouseDown(SceneDragSenderType::Enum inSenderType,
                                         QPoint inPoint, int)
{

}

void Q3DStudioRenderer::OnSceneMouseDrag(SceneDragSenderType::Enum, QPoint inPoint, int inToolMode,
                                         int inFlags)
{

}

void Q3DStudioRenderer::OnSceneMouseUp(SceneDragSenderType::Enum)
{

}

void Q3DStudioRenderer::OnSceneMouseDblClick(SceneDragSenderType::Enum inSenderType, QPoint inPoint)
{

}

void Q3DStudioRenderer::OnSceneMouseWheel(SceneDragSenderType::Enum inSenderType, short inDelta,
                                          int inToolMode)
{

}

void Q3DStudioRenderer::OnToolbarChange()
{

}

void Q3DStudioRenderer::OnSelectionChange()
{

}

qreal Q3DStudioRenderer::fixedDevicePixelRatio() const
{
    // Fix a problem on X11: https://bugreports.qt.io/browse/QTBUG-65570
    qreal ratio = m_widget->devicePixelRatio();
    if (QWindow *w = m_widget->window()->windowHandle()) {
        if (QScreen *s = w->screen())
            ratio = s->devicePixelRatio();
    }
    return ratio;
}

void Q3DStudioRenderer::sendResizeToQt3D(const QSize &size)
{
    if (!m_engine)
        return;
    Qt3DCore::QEntity *rootEntity = m_engine->rootEntity();
    if (rootEntity) {
        Qt3DRender::QRenderSurfaceSelector *surfaceSelector
                = Qt3DRender::QRenderSurfaceSelectorPrivate::find(rootEntity);

        if (surfaceSelector) {
            surfaceSelector->setExternalRenderTargetSize(size);
            surfaceSelector->setSurfacePixelRatio(float(fixedDevicePixelRatio()));
        }
    }
}

void Q3DStudioRenderer::createEngine()
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    QSurface *surface = context->surface();
    QOffscreenSurface *offscreen = nullptr;
    QWindow *window = nullptr;
    QObject *surfaceObject = nullptr;
    if (surface->surfaceClass() == QSurface::Offscreen) {
        offscreen = static_cast<QOffscreenSurface*>(surface);
        surfaceObject = offscreen;
    } else {
        window = static_cast<QWindow*>(surface);
        surfaceObject = window;
    }

    m_engine.reset(new Q3DSEngine);

    Q3DSEngine::Flags flags = Q3DSEngine::WithoutRenderAspect;
#if (Q3DS_ENABLE_PROFILEUI == 1)
    flags |= Q3DSEngine::EnableProfiling;
#endif

    m_viewportSettings.setMatteEnabled(true);
    m_viewportSettings.setShowRenderStats(true);
    QColor matteColor;
    matteColor.setRgbF(.13, .13, .13);
    m_viewportSettings.setMatteColor(matteColor);

    m_engine->setFlags(flags);
    m_engine->setAutoToggleProfileUi(false);

    m_engine->setSurface(surfaceObject);
    m_engine->setViewportSettings(&m_viewportSettings);

    m_renderAspect = new Qt3DRender::QRenderAspect(Qt3DRender::QRenderAspect::Synchronous);
    m_engine->createAspectEngine();
    m_engine->aspectEngine()->registerAspect(m_renderAspect);
}

void Q3DStudioRenderer::createTranslation()
{
    m_translation.reset(new Q3DSTranslation(*this));
}

std::shared_ptr<IStudioRenderer> IStudioRenderer::CreateStudioRenderer()
{
    return std::shared_ptr<IStudioRenderer>(new Q3DStudioRenderer());
}

}
