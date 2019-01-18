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
#include "StudioUtils.h"
#include "StudioPickValues.h"
#include "StudioFullSystem.h"
#include "StudioCoreSystem.h"
#include "HotKeys.h"
#include "Q3DSRenderBufferManager.h"

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qscreen.h>
#include <QtGui/qpainter.h>
#include <QtGui/qopenglpaintdevice.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qtimer.h>

#include <Qt3DRender/qrendersurfaceselector.h>
#include <Qt3DRender/private/qrendersurfaceselector_p.h>
#include <Qt3DCore/qentity.h>
#include <Qt3DCore/qaspectengine.h>
#include <Qt3DRender/qrenderaspect.h>
#include <Qt3DRender/private/qrenderaspect_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/qopenglframebufferobject.h>

namespace Q3DStudio {

const int g_wheelFactor = 10; // the wheel zoom factor

struct SEditCameraDefinition
{
    EditCameraTypes m_type;
    // Directional cameras have a direction they point
    QVector3D m_direction; // not normalized
    QString m_name;
};

static SEditCameraDefinition g_editCameraDefinitions[] = {
    { EditCameraTypes::Perspective, QVector3D(1, -1, -1), QObject::tr("Perspective View") },
    { EditCameraTypes::Orthographic, QVector3D(1, -1, -1), QObject::tr("Orthographic View") },
    { EditCameraTypes::Directional, QVector3D(0, -1, 0), QObject::tr("Top View") },
    { EditCameraTypes::Directional, QVector3D(0, 1, 0), QObject::tr("Bottom View") },
    { EditCameraTypes::Directional, QVector3D(1, 0, 0), QObject::tr("Left View") },
    { EditCameraTypes::Directional, QVector3D(-1, 0, 0), QObject::tr("Right View") },
    { EditCameraTypes::Directional, QVector3D(0, 0, 1), QObject::tr("Front View") },
    { EditCameraTypes::Directional, QVector3D(0, 0, -1), QObject::tr("Back View") },
};
static int g_numEditCameras = sizeof(g_editCameraDefinitions)
                                        / sizeof(*g_editCameraDefinitions);

Q3DStudioRenderer::Q3DStudioRenderer()
    : m_dispatch(*g_StudioApp.GetCore()->GetDispatch())
    , m_doc(*g_StudioApp.GetCore()->GetDoc())
    , m_updatableEditor(m_doc)
{
    m_dispatch.AddReloadListener(this);
    m_dispatch.AddDataModelListener(this);
    m_dispatch.AddPresentationChangeListener(this);
    m_selectionSignal
            = m_dispatch.ConnectSelectionChange(std::bind(&Q3DStudioRenderer::OnSelectionChange,
                                                          this));
    m_dispatch.AddSceneDragListener(this);
    m_dispatch.AddToolbarChangeListener(this);

    // Rectangles under tick marks
    m_rectColor = CStudioPreferences::GetRulerBackgroundColor();
    // Tick marks
    m_lineColor = CStudioPreferences::GetRulerTickColor();

    m_editCameraInformation.resize(g_numEditCameras);

    // Create engine and presentation as RenderBufferManager needs them before presentation is set
    m_engine.reset(new Q3DSEngine);
    m_presentation.reset(new Q3DSUipPresentation);
    m_viewportSettings.reset(new Q3DSViewportSettings);
}

Q3DStudioRenderer::~Q3DStudioRenderer()
{
    Close();
}

QSharedPointer<Q3DSEngine> &Q3DStudioRenderer::engine()
{
    return m_engine;
}

ITextRenderer *Q3DStudioRenderer::GetTextRenderer()
{
    return this;
}

QT3DSVec3 Q3DStudioRenderer::GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inHandle,
                                                 CPt inPoint)
{
    return QT3DSVec3();
}

Q3DSRenderBufferManager *Q3DStudioRenderer::GetBufferManager()
{
    return Q3DSRenderBufferManager::Create(m_engine.data(), m_presentation.data(),
                                           *IInputStreamFactory::Create()).data();
}

// Returns true if request is left pending.
// Returns false when picking from invalid spot or picking otherwise failed before it was queued.
bool Q3DStudioRenderer::requestObjectAt(const QPoint &pt)
{
    QPoint point;
    point.setX(int(pt.x() * StudioUtils::devicePixelRatio()));
    point.setY(int(pt.y() * StudioUtils::devicePixelRatio()));
    PickTargetAreas pickArea = getPickArea(point);
    if (pickArea == PickTargetAreas::Presentation) {
        SStudioPickValue pickValue = pick(point, SelectMode::Single, true);
        if (pickValue.getType() == StudioPickValueTypes::Pending) {
            RequestRender();
            return true;
        }
    }
    return false;
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
    return m_widget != nullptr;
}

void Q3DStudioRenderer::initialize(QOpenGLWidget *widget)
{
    m_widget = widget;

    if (m_widget && m_translation.isNull() && m_hasPresentation)
        initEngineAndTranslation();
}

void Q3DStudioRenderer::SetViewRect(const QRect &inRect, const QSize &size)
{
    m_viewRect = inRect;
    if (!m_engine.isNull() && m_widget && size != m_size) {
        m_engine->resize(size, false);
        m_resizeToQt3DSent = false;
        if (m_engine->sceneManager())
            m_engine->sceneManager()->uncacheLayers();
    }
    m_size = size;
    if (!m_resizeToQt3DSent)
        sendResizeToQt3D();
}

void Q3DStudioRenderer::setFullSizePreview(bool enabled)
{
    // TODO
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
    outCameras.clear();
    for (int idx = 0; idx < g_numEditCameras; ++idx)
        outCameras.push_back(g_editCameraDefinitions[idx].m_name);
}

bool Q3DStudioRenderer::DoesEditCameraSupportRotation(QT3DSI32 inIndex)
{
    if (inIndex >= 0 && inIndex < g_numEditCameras)
        return g_editCameraDefinitions[inIndex].m_type != EditCameraTypes::Directional;
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
    int index = qMin(inIndex, g_numEditCameras);
    m_pendingEditCameraIndex = index;
    if (index != m_editCameraIndex && !m_translation.isNull()) {
        // save old edit camera info
        if (editCameraEnabled())
            m_editCameraInformation[m_editCameraIndex] = m_translation->editCameraInfo();
        m_editCameraIndex = index;
        if (index == -1) {
            // use scene camera
            m_translation->disableEditCamera();
            m_viewportSettings->setMatteEnabled(true);
            m_translation->disableGradient();
        } else {
            // use edit camera
            m_viewportSettings->setMatteEnabled(false);
            const SEditCameraDefinition &def(g_editCameraDefinitions[m_editCameraIndex]);

            SEditCameraPersistentInformation &cameraInfo
                    = m_editCameraInformation[m_editCameraIndex];

            if (!cameraInfo.m_initialized) {
                QVector3D normalizedDir = def.m_direction;
                normalizedDir.normalize();
                if (def.m_type == EditCameraTypes::Directional) {
                    cameraInfo.m_direction = normalizedDir;
                } else {
                    cameraInfo.m_direction = normalizedDir;
                    cameraInfo.m_xRotation = qRadiansToDegrees(-qAtan(normalizedDir.x()
                                                                      / normalizedDir.z()));
                    cameraInfo.m_yRotation = qRadiansToDegrees(qAsin(normalizedDir.y()));
                }
                cameraInfo.m_name = def.m_name;
                cameraInfo.m_cameraType = def.m_type;
                cameraInfo.m_initialized = true;
            }

            m_translation->enableEditCamera(cameraInfo);
            m_translation->enableGradient();
        }
        RequestRender();
    }
}

QT3DSI32 Q3DStudioRenderer::GetEditCamera() const
{
    if (m_editCameraIndex >= 0 && m_editCameraIndex < g_numEditCameras)
        return m_editCameraIndex;
    return -1;
}

void Q3DStudioRenderer::EditCameraZoomToFit()
{

}

bool Q3DStudioRenderer::isMouseDown() const
{
    return m_mouseDown || m_scenePicker->state() == Q3DSScenePicker::Triggered
            || m_scenePicker->state() == Q3DSScenePicker::Queued;
}

void Q3DStudioRenderer::Close()
{
    OnClosingPresentation();
    m_engine.reset();
    m_presentation.reset();
    m_dispatch.RemoveDataModelListener(this);
    m_dispatch.RemovePresentationChangeListener(this);
    m_dispatch.RemoveSceneDragListener(this);
    m_dispatch.RemoveToolbarChangeListener(this);
    m_widget = nullptr;
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
    qreal length = CStudioPreferences::guideSize() / 2.;
    qreal centerPosX = floor(innerLeft + (innerRight - innerLeft) / 2.0 + .5);
    drawTopBottomTickMarks(painter, centerPosX, innerBottom, innerTop, outerBottom,
                           outerTop, 15. * length / 16.);
    for (unsigned int incrementor = 10;
         (centerPosX + incrementor) < innerRight && (centerPosX - incrementor) > innerLeft;
         incrementor += 10) {
        qreal rightEdge = centerPosX + incrementor;
        qreal leftEdge = centerPosX - incrementor;
        qreal lineHeight = 0;
        if (incrementor % 100 == 0)
            lineHeight = 11. * length / 16.;
        else if (incrementor % 20)
            lineHeight = 4. * length / 16.;
        else
            lineHeight = 2. * length / 16.;

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
    qreal length = CStudioPreferences::guideSize() / 2.;
    qreal centerPosY = floor(innerBottom + (innerTop - innerBottom) / 2.0 + .5);
    drawLeftRightTickMarks(painter, centerPosY, innerLeft, innerRight, outerLeft,
                           outerRight, 15. * length / 16.);
    for (unsigned int incrementor = 10;
         (centerPosY + incrementor) < innerTop && (centerPosY - incrementor) > innerBottom;
         incrementor += 10) {
        qreal topEdge = centerPosY + incrementor;
        qreal bottomEdge = centerPosY - incrementor;
        qreal lineHeight = 0;
        if (incrementor % 100 == 0)
            lineHeight = 11. * length / 16.;
        else if (incrementor % 20)
            lineHeight = 4. * length / 16.;
        else
            lineHeight = 2. * length / 16.;

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

void Q3DStudioRenderer::drawGuides(QPainter &painter)
{
    if (!m_guidesEnabled || editCameraEnabled())
        return;

    int offset = CStudioPreferences::guideSize() / 2;

    int innerLeft = m_viewRect.left() + offset;
    int innerRight = m_viewRect.right() - offset + 1;
    int innerBottom = m_viewRect.bottom() - offset + 1;
    int innerTop = m_viewRect.top() + offset;

    int outerLeft = innerLeft - offset;
    int outerRight = innerRight + offset;
    int outerBottom = innerBottom + offset;
    int outerTop = innerTop - offset;

    // Retain the rects for picking purposes.
    m_innerRect = QRect(innerLeft, innerTop, innerRight - innerLeft, innerBottom - innerTop);
    m_outerRect = QRect(outerLeft, outerTop, outerRight - outerLeft, outerBottom - outerTop);

    // Draw tick marks around the presentation
    painter.fillRect(QRect(outerLeft, outerTop,
                           innerLeft - outerLeft, outerBottom - outerTop),
                     m_rectColor);
    painter.fillRect(QRect(innerRight, outerTop,
                           outerRight - innerRight, outerBottom - outerTop),
                     m_rectColor);
    painter.fillRect(QRect(innerLeft, innerBottom,
                           innerRight - innerLeft, outerBottom - innerBottom),
                     m_rectColor);
    painter.fillRect(QRect(innerLeft, outerTop,
                           innerRight - innerLeft, innerTop - outerTop),
                     m_rectColor);

    painter.setPen(m_lineColor);
    drawTickMarksOnHorizontalRects(painter, innerLeft, innerRight + 1, innerTop - 1, innerBottom,
                                   outerTop, outerBottom);

    drawTickMarksOnVerticalRects(painter, innerLeft, innerRight + 1, innerTop - 1, innerBottom,
                                 outerLeft, outerRight);
}

void Q3DStudioRenderer::renderNow()
{
    if (m_setSubpresentationsCalled == false)
        return;

    initialize(m_widget);

    m_renderRequested = false;
    QOpenGLContextPrivate *ctxD = QOpenGLContextPrivate::get(m_widget->context());
    QScopedValueRollback<GLuint> defaultFboRedirectRollback(ctxD->defaultFboRedirect, 0);

    if (!m_translation.isNull()) {
        // We are always rendering into a fbo with 1x pixel ratio. The scene widget will
        // display it a proper size.
        m_translation->prepareRender(QRect(0, 0, m_size.width(), m_size.height()), m_size, 1.0);

        // m_translation->prepareRender clears context, so make sure to activate it again
        if (!QOpenGLContext::currentContext())
            m_widget->makeCurrent();

        auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                    Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        renderAspectD->renderSynchronous(true);

        // fix gl state leakage
        QOpenGLContext::currentContext()->functions()->glDisable(GL_STENCIL_TEST);
        QOpenGLContext::currentContext()->functions()->glDisable(GL_DEPTH_TEST);
        QOpenGLContext::currentContext()->functions()->glDisable(GL_CULL_FACE);
        QOpenGLContext::currentContext()->functions()->glDisable(GL_SCISSOR_TEST);
        QOpenGLContext::currentContext()->functions()->glDisable(GL_BLEND);
    }

    // TODO: Guides drawing is not working
    QOpenGLPaintDevice device;
    device.setSize(m_size);
    QPainter painter(&device);

    // draw guides if enabled
    drawGuides(painter);
}

void Q3DStudioRenderer::getPreviewFbo(QSize &outFboDim, qt3ds::QT3DSU32 &outFboTexture)
{
#ifdef RUNTIME_SPLIT_TEMPORARILY_REMOVED
    if (m_Translation) {
        outFboDim = QSize(m_Translation->m_previewFboDimensions.x,
                          m_Translation->m_previewFboDimensions.y);
        // The handle is a void * so first cast to size_t to avoid truncating pointer warning
        if (m_Translation->m_previewTexture) {
            outFboTexture = static_cast<qt3ds::QT3DSU32>(reinterpret_cast<size_t>(
                        m_Translation->m_previewTexture->GetTextureObjectHandle()));
        } else {
            outFboTexture = 0;
        }

    } else {
        outFboDim = QSize(0, 0);
        outFboTexture = 0;
    }
#endif
}

void Q3DStudioRenderer::RegisterSubpresentations(
        const QVector<SubPresentationRecord> &subpresentations){
    m_subpresentations = subpresentations;
    m_setSubpresentationsCalled = true;
}

void Q3DStudioRenderer::OnBeginDataModelNotifications()
{

}

void Q3DStudioRenderer::OnEndDataModelNotifications()
{

}

void Q3DStudioRenderer::OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{
    if (!m_translation.isNull()) {
        m_translation->markDirty(inInstance);
        RequestRender();
    }
}

void Q3DStudioRenderer::OnImmediateRefreshInstanceMultiple(
        qt3dsdm::Qt3DSDMInstanceHandle *inInstance, long inInstanceCount)
{
    if (!m_translation.isNull()) {
        m_translation->markDirty(inInstance, inInstanceCount);
        RequestRender();
    }
}

void Q3DStudioRenderer::OnReloadEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
{

}

void Q3DStudioRenderer::OnNewPresentation()
{
    m_hasPresentation = true;
}

void Q3DStudioRenderer::OnClosingPresentation()
{
    if (!m_hasPresentation || !m_widget)
        return;

    if (!m_engine.isNull() && !m_translation.isNull()) {
        m_widget->makeCurrent();
        auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                    Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        renderAspectD->renderShutdown();
        m_widget->doneCurrent();
        m_renderAspect = nullptr;

        // This will destroy render aspect
        m_engine->setPresentation(nullptr);
        m_translation.reset();
        m_engine.reset(new Q3DSEngine);
        m_presentation.reset(new Q3DSUipPresentation);
        m_viewportSettings.reset(new Q3DSViewportSettings);
    }
    m_hasPresentation = false;
}

PickTargetAreas Q3DStudioRenderer::getPickArea(const QPoint &point)
{
    const int pickPointX(point.x());
    const int pickPointY(point.y());
    const int offset = !editCameraEnabled() ? CStudioPreferences::guideSize() / 2 : 0;
    QRect rect = QRect(offset + m_viewRect.left(), + m_viewRect.top() + offset,
                       m_viewRect.width() - 2 * offset,
                       m_viewRect.height() - 2 * offset);
    if (pickPointX < rect.left() || pickPointX > rect.right()
        || pickPointY < rect.top() || pickPointY > rect.bottom()) {
        return PickTargetAreas::Matte;
    }
    return PickTargetAreas::Presentation;
}

QPoint Q3DStudioRenderer::scenePoint(const QPoint &viewPoint)
{
    // TODO: fix point mapping according to current screen pixel ratio
    // TODO: fix camera offset when edge guides are added
    //int offset = !editCameraEnabled() ? CStudioPreferences::guideSize() / 2 : 0;
    int offset = 0;
    return QPoint(viewPoint.x() - offset - m_viewRect.left(),
                  viewPoint.y() - offset - m_viewRect.top());
}

qt3ds::foundation::Option<qt3dsdm::SGuideInfo> Q3DStudioRenderer::pickRulers(CPt inMouseCoords)
{
    CPt renderSpacePt(inMouseCoords.x, long(m_viewRect.y()) - inMouseCoords.y);
    // If mouse is inside outer rect but outside inner rect.
    if (m_outerRect.contains(renderSpacePt.x, renderSpacePt.y)
        && !m_innerRect.contains(renderSpacePt.x, renderSpacePt.y)) {
        std::shared_ptr<qt3dsdm::IGuideSystem> theGuideSystem =
            g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystem()
                ->GetCoreSystem()->GetGuideSystem();
        if (renderSpacePt.x >= m_innerRect.left() && renderSpacePt.x <= m_innerRect.right()) {
            return qt3dsdm::SGuideInfo(renderSpacePt.y - m_innerRect.bottom(),
                                     qt3dsdm::GuideDirections::Horizontal);
        } else if (renderSpacePt.y >= m_innerRect.bottom()
                   && renderSpacePt.y <= m_innerRect.top()) {
            return qt3dsdm::SGuideInfo(renderSpacePt.x - m_innerRect.left(),
                                     qt3dsdm::GuideDirections::Vertical);
        }
    }
    return qt3ds::foundation::Option<qt3dsdm::SGuideInfo>();
}

SStudioPickValue Q3DStudioRenderer::pick(const QPoint &inMouseCoords, SelectMode inSelectMode,
                                         bool objectPick)
{
    QPoint renderPoint = scenePoint(inMouseCoords);

    if (m_doc.GetDocumentReader().AreGuidesEditable()) {
        qt3dsdm::TGuideHandleList theGuides = m_doc.GetDocumentReader().GetGuides();
        for (size_t guideIdx = 0, guideEnd = theGuides.size(); guideIdx < guideEnd; ++guideIdx) {
            qt3dsdm::SGuideInfo theGuideInfo =
                m_doc.GetDocumentReader().GetGuideInfo(theGuides[guideIdx]);
            float width = (theGuideInfo.m_Width / 2.0f) + 2.0f;
            switch (theGuideInfo.m_Direction) {
            case qt3dsdm::GuideDirections::Horizontal:
                if (qAbs(float(renderPoint.y()) - theGuideInfo.m_Position) <= width)
                    return theGuides[guideIdx];
                break;
            case qt3dsdm::GuideDirections::Vertical:
                if (qAbs(float(renderPoint.x()) - theGuideInfo.m_Position) <= width)
                    return theGuides[guideIdx];
                break;
            default:
                break;
            }
        }
    }

    // Pick against the widget first if possible.
    // TODO:: widgets
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    if (m_LastRenderedWidget && (m_LastRenderedWidget->GetNode().m_Flags.IsActive()
                                 || m_LastRenderedWidget->GetNode().m_Type
                                 == GraphObjectTypes::Light
                                 || m_LastRenderedWidget->GetNode().m_Type
                                 == GraphObjectTypes::Camera)) {
        Option<QT3DSU32> picked = PickWidget(inMouseCoords, inSelectMode, *m_LastRenderedWidget);
        if (picked.hasValue()) {
            RequestRender();
            DoPrepareForDrag(&m_LastRenderedWidget->GetNode());
            return m_LastRenderedWidget->PickIndexToPickValue(*picked);
        }
    }
    // Pick against Lights and Cameras
    // This doesn't use the color picker or renderer pick
    float lastDist = 99999999999999.0f;
    int lastIndex = -1;
    for (int i = 0; i < int(m_editModeCamerasAndLights.size()); ++i) {
        const QT3DSVec2 mouseCoords((QT3DSF32)inMouseCoords.x, (QT3DSF32)inMouseCoords.y);
        float dist;
        SGraphObject &object = m_editModeCamerasAndLights[i]->GetGraphObject();
        m_VisualAidWidget->SetNode(static_cast<SNode *>(&object));
        if (m_VisualAidWidget->pick(m_Context.GetRenderer().GetRenderWidgetContext(),
                                    dist, GetViewportDimensions(), mouseCoords)) {
            if (dist < lastDist) {
                lastDist = dist;
                lastIndex = i;
            }
        }
    }
#endif

    if (!m_translation.isNull()) {
        ensurePicker();
        if (m_scenePicker->state() == Q3DSScenePicker::Unqueued) {
            m_objectPicking = objectPick;
            m_scenePicker->pick(renderPoint);
            if (m_scenePicker->state() == Q3DSScenePicker::Failed) {
                m_scenePicker->reset();
                return SStudioPickValue();
            } else {
                return SStudioPickValue(StudioPickValueTypes::Pending);
            }
        }
    }

#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    if (lastIndex != -1) {
        DoPrepareForDrag(static_cast<SNode *>(
                             &(m_editModeCamerasAndLights[lastIndex]->GetGraphObject())));
        return m_editModeCamerasAndLights[lastIndex]->GetInstanceHandle();
    }
#endif
    return SStudioPickValue();
}

void Q3DStudioRenderer::ensurePicker()
{
    if (m_scenePicker.isNull()) {
        m_scenePicker.reset(new Q3DSScenePicker(m_engine->sceneManager()));
        QObject::connect(m_scenePicker.data(), &Q3DSScenePicker::ready, this,
                         &Q3DStudioRenderer::onScenePick, Qt::QueuedConnection);
    }
}

void Q3DStudioRenderer::onScenePick()
{
    if (m_scenePicker->state() == Q3DSScenePicker::Ready) {
        SStudioPickValue pickResult;
        if (m_scenePicker->isHit())
            pickResult = postScenePick(m_objectPicking);
        handlePickResult(pickResult, m_objectPicking);
    }
    m_scenePicker->reset();
    m_objectPicking = false;
}

SStudioPickValue Q3DStudioRenderer::postScenePick(bool objectPick)
{
    Q3DSGraphObject *object = m_scenePicker->objects().first();
    const qreal distance = m_scenePicker->distances().first();
    Q3DSGraphObjectTranslator *translator = Q3DSGraphObjectTranslator::translatorForObject(object);

#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    // check hit distance to cameras and lights
    if (lastIndex != -1 && thePickResult.m_CameraDistanceSq > lastDist * lastDist) {
        DoPrepareForDrag(static_cast<SNode *>(
                             &(m_editModeCamerasAndLights[lastIndex]->GetGraphObject())));
        return m_editModeCamerasAndLights[lastIndex]->GetInstanceHandle();
    }
#endif

    if (object->type() == Q3DSGraphObject::Model || object->type() == Q3DSGraphObject::Text) {
        if (translator->possiblyAliasedInstanceHandle() != translator->instanceHandle()) {
            translator = m_translation->getOrCreateTranslator(
                            translator->possiblyAliasedInstanceHandle());
        }
        bool groupSelectMode = (g_StudioApp.GetSelectMode() == STUDIO_SELECTMODE_GROUP);

        Qt3DSDMInstanceHandle theActiveComponent
                = m_translation->reader().GetComponentForSlide(m_doc.GetActiveSlide());
        if (groupSelectMode) {
            // Bounce up the hierarchy till one of two conditions are met
            // the parent is a layer or the our component is the active component
            // but the parent's is not.
            while (translator && translator->graphObject().isNode()) {
                Q3DSGraphObject *node =  &translator->graphObject();
                if (node->parent() == nullptr) {
                    translator = nullptr;
                    break;
                }
                Q3DSGraphObject *parentNode = node->parent();
                Q3DSGraphObjectTranslator *parentTranslator
                        = Q3DSGraphObjectTranslator::translatorForObject(parentNode);
                Qt3DSDMInstanceHandle myComponent
                        = m_translation->reader().GetAssociatedComponent(
                            translator->instanceHandle());
                Qt3DSDMInstanceHandle myParentComponent
                        = m_translation->reader()
                            .GetAssociatedComponent(parentTranslator->instanceHandle());
                if (parentNode->type() == Q3DSGraphObject::Layer) {
                    if (myParentComponent != theActiveComponent)
                        translator = nullptr;
                    break;
                }
                if (myComponent == theActiveComponent && myParentComponent != theActiveComponent)
                    break;
                translator = parentTranslator;
            }
        } else {
            // Bounce up until we get into the active component and then stop.
            while (translator && translator->graphObject().isNode()
                   && m_translation->reader().GetAssociatedComponent(translator->instanceHandle())
                       != theActiveComponent) {
                Q3DSGraphObject *node = &translator->graphObject();
                node = node->parent();
                if (node && node->type() != Q3DSGraphObject::Layer)
                    translator = Q3DSGraphObjectTranslator::translatorForObject(node);
                else
                    translator = nullptr;
            }
        }
    }

    if (translator) {
        Q_ASSERT(translator->graphObject().isNode());
        if (!objectPick)
            m_translation->prepareDrag(translator);
        return translator->instanceHandle();
    }
    return SStudioPickValue();
}

void Q3DStudioRenderer::handlePickResult(const SStudioPickValue &pickResult, bool objectPick)
{
    int pickedInstance = 0;
    if (!objectPick)
        m_dragPickResult = pickResult;
    switch (pickResult.getType()) {
    case StudioPickValueTypes::Instance: {
        qt3dsdm::Qt3DSDMInstanceHandle theHandle(pickResult.getData<Qt3DSDMInstanceHandle>());
        if (objectPick) {
            pickedInstance = theHandle.GetHandleValue();
        } else {
            if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
                m_doc.ToggleDataModelObjectToSelection(theHandle);
            } else {
                if (m_doc.getSelectedInstancesCount() > 1)
                    m_doc.DeselectAllItems(true);

                if (theHandle != m_doc.GetSelectedInstance())
                    m_doc.SelectDataModelObject(theHandle);
            }
        }
        break;
    }
    case StudioPickValueTypes::Guide: {
        if (!objectPick)
            m_doc.NotifySelectionChanged(pickResult.getData<qt3dsdm::Qt3DSDMGuideHandle>());
        break;
    }
    case StudioPickValueTypes::UnknownValueType: {
        if (!objectPick)
            m_doc.DeselectAllItems(true);
        break;
    }
    case StudioPickValueTypes::Pending:
        qWarning() << __FUNCTION__ << "Got a pending result for a pick";
        break;
    default:
        break;
    }
    if (objectPick)
        Q_EMIT objectPicked(pickedInstance);
}

void Q3DStudioRenderer::OnSceneMouseDown(SceneDragSenderType::Enum inSenderType,
                                         QPoint inPoint, int)
{
    if (m_translation.isNull())
        return;

    m_mouseDown = true;

    inPoint.setX(int(inPoint.x() * StudioUtils::devicePixelRatio()));
    inPoint.setY(int(inPoint.y() * StudioUtils::devicePixelRatio()));

    m_dragPickResult = SStudioPickValue();
    if (inSenderType == SceneDragSenderType::SceneWindow) {
        PickTargetAreas pickArea = getPickArea(inPoint);
        if (pickArea == PickTargetAreas::Presentation) {
            SelectMode theSelectMode = SelectMode::Group;
            switch (g_StudioApp.GetSelectMode()) {
            case STUDIO_SELECTMODE_ENTITY:
                theSelectMode = SelectMode::Single;
                break;
            case STUDIO_SELECTMODE_GROUP:
                theSelectMode = SelectMode::Group;
                break;
            default:
                Q_ASSERT_X(false, __FUNCTION__, "Invalid selection mode");
                break;
            }

            m_dragPickResult = pick(inPoint, theSelectMode, false);
            RequestRender();

        } else if (pickArea == PickTargetAreas::Matte) {
            qt3ds::foundation::Option<qt3dsdm::SGuideInfo> pickResult = pickRulers(inPoint);
            if (pickResult.hasValue()) {
                Q3DStudio::IDocumentEditor &docEditor(
                            m_updatableEditor.EnsureEditor(QObject::tr("Create Guide"),
                                                           __FILE__, __LINE__));
                qt3dsdm::Qt3DSDMGuideHandle newGuide = docEditor.CreateGuide(*pickResult);
                m_dragPickResult = SStudioPickValue(newGuide);
                m_doc.NotifySelectionChanged(newGuide);
            } else {
                m_doc.DeselectAllItems(true);
            }
        }
    }

    m_lastDragToolMode = MovementTypes::Unknown;
    m_maybeDragStart = true;
    m_mouseDownPoint = inPoint;
    m_previousMousePoint = inPoint;
    m_mouseDownCameraInformation = m_translation->editCameraInfo();
    m_lastToolMode = g_StudioApp.GetToolMode();
}

SEditCameraPersistentInformation panEditCamera(const QPoint &pan,
                                               const QVector3D &position,
                                               const SEditCameraPersistentInformation &theInfo,
                                               const QSizeF &viewport)
{
    SEditCameraPersistentInformation info(theInfo);
    QVector3D theXAxis = info.left();
    QVector3D theYAxis = info.up();
    QVector3D theXChange = -1.0f * theXAxis * pan.x() * info.zoomFactor(viewport);
    QVector3D theYChange = theYAxis * pan.y() * info.zoomFactor(viewport);
    QVector3D theDiff = theXChange + theYChange;
    info.m_position = position + theDiff;
    return info;
}

SEditCameraPersistentInformation rotateEditCamera(const QPoint &distance,
                                                  qreal xrot, qreal yrot,
                                                  const SEditCameraPersistentInformation &theInfo)
{
    SEditCameraPersistentInformation info(theInfo);
    info.m_xRotation = xrot + (qreal(distance.x()) * g_rotationScaleFactor / 20.);
    info.m_yRotation = yrot - (qreal(distance.y()) * g_rotationScaleFactor / 20.);

    // Avoid rounding errors stemming from extremely large rotation angles
    if (info.m_xRotation < -180.)
        info.m_xRotation += 360.;
    if (info.m_xRotation > 180.)
        info.m_xRotation -= 360.;
    if (info.m_yRotation < -180.)
        info.m_yRotation += 360.;
    if (info.m_yRotation > 180.)
        info.m_yRotation -= 360.;
    return info;
}

void Q3DStudioRenderer::OnSceneMouseDrag(SceneDragSenderType::Enum, QPoint inPoint, int inToolMode,
                                         int inFlags)
{
    // skip event if we are not ready
    if (m_translation.isNull() || m_dragPickResult.getType() == StudioPickValueTypes::Pending)
        return;

    inPoint.setX(int(inPoint.x() * StudioUtils::devicePixelRatio()));
    inPoint.setY(int(inPoint.y() * StudioUtils::devicePixelRatio()));

    if (m_maybeDragStart) {
        QPoint theDragDistance = inPoint - m_mouseDownPoint;
        const int dragDistanceLimit = QApplication::startDragDistance();
        if (m_dragPickResult.getType() == StudioPickValueTypes::Widget
            || inToolMode != STUDIO_TOOLMODE_SCALE) {
            if (theDragDistance.manhattanLength() <= dragDistanceLimit)
                return;
        } else {
            if (qAbs(theDragDistance.y()) <= dragDistanceLimit)
                return;
        }
    }

    m_maybeDragStart = false;

    // If the tool mode changes then we throw out the last widget pick if there was one.
    if (m_lastToolMode != inToolMode)
        m_dragPickResult = SStudioPickValue();
    m_lastToolMode = inToolMode;

    // General dragging
    if (m_dragPickResult.getType() == StudioPickValueTypes::Instance
        || m_dragPickResult.getType()
            == StudioPickValueTypes::UnknownValueType) // matte drag and widget drag
    {
        // Not sure what right-click drag does in the scene.
        const bool isEditCamera = editCameraEnabled();
        const int theCameraToolMode = isEditCamera ? (inToolMode & (STUDIO_CAMERATOOL_MASK)) : 0;
        const bool rightClick = (inFlags & CHotKeys::MOUSE_RBUTTON) != 0;

        if (theCameraToolMode == 0) {
            if (m_doc.GetDocumentReader().IsInstance(m_doc.GetSelectedInstance())) {
                if (m_doc.getSelectedInstancesCount() == 1) {
                    bool rightClick = (inFlags & CHotKeys::MOUSE_RBUTTON) != 0;
                    MovementTypes theMovement(MovementTypes::Unknown);

                    switch (inToolMode) {
                    case STUDIO_TOOLMODE_MOVE:
                        if (rightClick)
                            theMovement = MovementTypes::TranslateAlongCameraDirection;
                        else
                            theMovement = MovementTypes::Translate;
                        break;
                    case STUDIO_TOOLMODE_SCALE:
                        if (rightClick)
                            theMovement = MovementTypes::ScaleZ;
                        else
                            theMovement = MovementTypes::Scale;
                        break;
                    case STUDIO_TOOLMODE_ROTATE:
                        if (rightClick)
                            theMovement = MovementTypes::RotationAboutCameraDirection;
                        else
                            theMovement = MovementTypes::Rotation;
                        break;
                    default:
                        Q_ASSERT_X(false, __FUNCTION__, "invalid toolmode");
                        break;
                    }

                    if (theMovement != MovementTypes::Unknown) {
                        bool theLockToAxis = (inFlags & CHotKeys::MODIFIER_SHIFT) != 0;

                        if (m_lastDragToolMode != MovementTypes::Unknown
                            && theMovement != m_lastDragToolMode) {
                            m_updatableEditor.RollbackEditor();
                            m_mouseDownPoint = inPoint;
                        }

                        m_lastDragToolMode = theMovement;

                        switch (theMovement) {
                        case MovementTypes::TranslateAlongCameraDirection:
                            m_translation->translateAlongCameraDirection(m_mouseDownPoint, inPoint,
                                                                         m_updatableEditor);
                            break;
                        case MovementTypes::Translate:
                            m_translation->translate(m_mouseDownPoint, inPoint, m_updatableEditor,
                                                     theLockToAxis);
                            break;
                        case MovementTypes::ScaleZ:
                            m_translation->scaleZ(m_mouseDownPoint, inPoint, m_updatableEditor);
                            break;
                        case MovementTypes::Scale:
                            m_translation->scale(m_mouseDownPoint, inPoint, m_updatableEditor);
                            break;
                        case MovementTypes::Rotation:
                            m_translation->rotate(m_mouseDownPoint, inPoint,
                                                  m_updatableEditor, theLockToAxis);
                            break;
                        case MovementTypes::RotationAboutCameraDirection:
                            m_translation->rotateAboutCameraDirectionVector(m_mouseDownPoint,
                                                inPoint, m_updatableEditor);
                            break;
                        default:
                            Q_ASSERT_X(false, __FUNCTION__, "invalid movement mode");
                            break;
                        }

                    }
                }
            }
        } else {
            QPoint distance = inPoint - m_mouseDownPoint;
            QPoint subsetDistance = inPoint - m_previousMousePoint;

            switch (theCameraToolMode) {
            case STUDIO_TOOLMODE_CAMERA_PAN: {
                m_translation->enableEditCamera(panEditCamera(distance,
                                                              m_mouseDownCameraInformation.m_position,
                                                              m_translation->editCameraInfo(),
                                                              m_viewRect.size()));
                RequestRender();
            } break;
            case STUDIO_TOOLMODE_CAMERA_ZOOM: {
                qreal theMultiplier = 1.0 + qreal(subsetDistance.y()) / 40.0;
                m_translation->wheelZoom(theMultiplier);
                RequestRender();
            } break;
            case STUDIO_TOOLMODE_CAMERA_ROTATE: {
                if (m_translation->editCameraInfo().supportsRotation()) {
                    if (!rightClick) {
                        m_translation->enableEditCamera(
                            rotateEditCamera(subsetDistance,
                                             m_mouseDownCameraInformation.m_xRotation,
                                             m_mouseDownCameraInformation.m_yRotation,
                                             m_translation->editCameraInfo()));
                    }
                    m_mouseDownCameraInformation.m_xRotation =
                        m_translation->editCameraInfo().m_xRotation;
                    m_mouseDownCameraInformation.m_yRotation =
                        m_translation->editCameraInfo().m_yRotation;
                    RequestRender();
                }
            } break;
            default:
                break;
            }
        }
    } // if ( m_PickResult.m_WidgetId.hasValue() == false )
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    // We need to do widget-specific dragging.
    else if (m_pickResult.getType() == StudioPickValueTypes::Widget) {
        m_Translation->PerformWidgetDrag(m_PickResult.GetWidgetId(), m_MouseDownPoint,
                                         m_PreviousMousePoint, inPoint, m_UpdatableEditor);
    } else if (m_PickResult.getType() == StudioPickValueTypes::Guide) {
        m_Translation->PerformGuideDrag(m_PickResult.getData<Qt3DSDMGuideHandle>(), inPoint,
                                        m_UpdatableEditor);
    } else if (m_PickResult.getType() == StudioPickValueTypes::Path) {
        SPathPick thePick = m_PickResult.getData<SPathPick>();
        m_Translation->PerformPathDrag(thePick, m_MouseDownPoint, m_PreviousMousePoint, inPoint,
                                       m_UpdatableEditor);
    }
#endif
    m_previousMousePoint = inPoint;
}

void Q3DStudioRenderer::OnSceneMouseUp(SceneDragSenderType::Enum)
{
    m_maybeDragStart = false;
    m_mouseDown = false;
    qt3dsdm::Qt3DSDMGuideHandle theSelectedGuide;
    if (m_dragPickResult.getType() == StudioPickValueTypes::Guide) {
        theSelectedGuide = m_dragPickResult.getData<qt3dsdm::Qt3DSDMGuideHandle>();
        // TODO:
        //m_Translation->CheckGuideInPresentationRect(theSelectedGuide, m_UpdatableEditor);
    }
    if (m_lastDragToolMode != MovementTypes::Unknown)
        m_translation->endDrag(false, m_updatableEditor);
    m_updatableEditor.CommitEditor();
    m_dragPickResult = SStudioPickValue();

    if (theSelectedGuide.GetHandleValue()) {
        // Get rid of selection if things aren't editable.
        if (m_doc.GetDocumentReader().AreGuidesEditable())
            m_doc.NotifySelectionChanged(theSelectedGuide);
        else
            m_doc.NotifySelectionChanged();
    }
    RequestRender();
}

void Q3DStudioRenderer::OnSceneMouseDblClick(SceneDragSenderType::Enum inSenderType, QPoint inPoint)
{

}

void Q3DStudioRenderer::OnSceneMouseWheel(SceneDragSenderType::Enum inSenderType, short inDelta,
                                          int inToolMode)
{
    Q_ASSERT(inSenderType == SceneDragSenderType::Matte);
    if (inToolMode == STUDIO_TOOLMODE_CAMERA_ZOOM && m_translation.data()) {
        qreal theMultiplier = 1.0 - inDelta / qreal(120 * g_wheelFactor);
        m_translation->wheelZoom(theMultiplier);
        RequestRender();
    }
}

void Q3DStudioRenderer::OnToolbarChange()
{

}

void Q3DStudioRenderer::OnSelectionChange()
{

}

void Q3DStudioRenderer::sendResizeToQt3D()
{
    if (m_engine.isNull())
        return;
    Qt3DCore::QEntity *rootEntity = m_engine->rootEntity();
    if (rootEntity) {
        Qt3DRender::QRenderSurfaceSelector *surfaceSelector
                = Qt3DRender::QRenderSurfaceSelectorPrivate::find(rootEntity);

        if (surfaceSelector) {
            surfaceSelector->setExternalRenderTargetSize(m_size);
            // We are always rendering into an fbo with 1x pixel ratio
            surfaceSelector->setSurfacePixelRatio(1.0f);
            m_resizeToQt3DSent = true;
            RequestRender();
        }
    }
}

void Q3DStudioRenderer::initEngineAndTranslation()
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    QSurface *surface = context->surface();
    QWindow *window = nullptr;
    QOffscreenSurface *offscreen = nullptr;
    QObject *surfaceObject = nullptr;
    if (surface->surfaceClass() == QSurface::Offscreen) {
        offscreen = static_cast<QOffscreenSurface *>(surface);
        surfaceObject = offscreen;
    } else {
        window = static_cast<QWindow *>(surface);
        surfaceObject = window;
    }

    m_scenePicker.reset();

    Q3DSEngine::Flags flags = Q3DSEngine::WithoutRenderAspect;

    if (CStudioApp::hasProfileUI())
        flags |= Q3DSEngine::EnableProfiling;

    m_viewportSettings->setMatteEnabled(true);
    m_viewportSettings->setShowRenderStats(false);
    QColor matteColor;
    matteColor.setRgbF(.13, .13, .13);
    m_viewportSettings->setMatteColor(matteColor);

    m_engine->setFlags(flags);
    m_engine->setAutoToggleProfileUi(false);

    m_engine->setSurface(surfaceObject);
    m_engine->setViewportSettings(m_viewportSettings.data());

    m_renderAspect = new Qt3DRender::QRenderAspect(Qt3DRender::QRenderAspect::Synchronous);
    m_engine->createAspectEngine();
    m_engine->aspectEngine()->registerAspect(m_renderAspect);

    auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
    renderAspectD->renderInitialize(m_widget->context());

    createTranslation();
    setupTextRenderer();

    if (m_editCameraIndex != m_pendingEditCameraIndex)
        SetEditCamera(m_pendingEditCameraIndex);
}

void Q3DStudioRenderer::createTranslation()
{
    m_translation.reset(new Q3DSTranslation(*this, m_presentation));
}

void Q3DStudioRenderer::reloadFonts()
{
    setupTextRenderer();
}

void Q3DStudioRenderer::setupTextRenderer()
{
    if (!m_engine.isNull() && m_engine->sceneManager()) {
        m_systemFonts.clear();
        m_projectFonts.clear();
        Q3DSTextRenderer *tr = m_engine->sceneManager()->textRenderer();
        tr->clearFonts();

        QString projectPath = g_StudioApp.GetCore()->getProjectFile().getProjectPath();
        if (!projectPath.isEmpty()) {
            // Add the installed font folders from the res dir.
            QString thePath(StudioUtils::resourcePath() + QStringLiteral("/Font"));
            m_systemFonts.push_back(thePath);
            m_projectFonts.push_back(projectPath);
            tr->registerFonts({thePath, projectPath});
        }
    }
}

QVector<SFontEntry> &Q3DStudioRenderer::projectFontList()
{
    return m_fonts;
}

void Q3DStudioRenderer::scheduleDirtySetUpdate()
{
    if (!m_dirtySetUpdate) {
        m_dirtySetUpdate = true;
        QTimer::singleShot(0, [this]() {
            m_translation->clearDirtySet();
            m_dirtySetUpdate = false;
        });
    }
}

std::shared_ptr<IStudioRenderer> IStudioRenderer::CreateStudioRenderer()
{
    return std::shared_ptr<IStudioRenderer>(new Q3DStudioRenderer());
}

}
