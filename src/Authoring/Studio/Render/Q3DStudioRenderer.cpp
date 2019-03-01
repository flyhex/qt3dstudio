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
#include <Qt3DCore/private/qaspectengine_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/qopenglframebufferobject.h>

namespace Q3DStudio {

static const int rulerTickInterval = 10;
static const int wheelZoomFactor = 10;

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
    { EditCameraTypes::Directional, QVector3D(-1, 0, 0), QObject::tr("Left View") },
    { EditCameraTypes::Directional, QVector3D(1, 0, 0), QObject::tr("Right View") },
    { EditCameraTypes::Directional, QVector3D(0, 0, 1), QObject::tr("Front View") },
    { EditCameraTypes::Directional, QVector3D(0, 0, -1), QObject::tr("Back View") },
};
static int g_numEditCameras = sizeof(g_editCameraDefinitions)
                                        / sizeof(*g_editCameraDefinitions);

Q3DStudioRenderer::Q3DStudioRenderer(bool sceneCameraMode)
    : m_dispatch(*g_StudioApp.GetCore()->GetDispatch())
    , m_doc(*g_StudioApp.GetCore()->GetDoc())
    , m_guidesEnabled(!sceneCameraMode)
    , m_updatableEditor(m_doc)
    , m_sceneCameraMode(sceneCameraMode)
{
    m_dispatch.AddReloadListener(this);
    m_dispatch.AddDataModelListener(this);
    m_dispatch.AddPresentationChangeListener(this);
    if (!m_sceneCameraMode) {
        m_selectionSignal
                = m_dispatch.ConnectSelectionChange(std::bind(&Q3DStudioRenderer::onSelectionChange,
                                                              this, std::placeholders::_1));
        m_dispatch.AddSceneDragListener(this);
        m_dispatch.AddToolbarChangeListener(this);

        m_rulerColor = CStudioPreferences::GetRulerBackgroundColor();
        m_rulerTickColor = CStudioPreferences::GetRulerTickColor();
        m_guideColor = CStudioPreferences::GetGuideColor();
        m_guideSelectedColor = CStudioPreferences::GetGuideSelectedColor();
        m_guideFillColor = CStudioPreferences::GetGuideFillColor();
        m_guideSelectedFillColor = CStudioPreferences::GetGuideFillSelectedColor();

        m_editCameraInformation.resize(g_numEditCameras);
    }

    // Create engine and presentation as RenderBufferManager needs them before presentation is set
    m_engine.reset(new Q3DSEngine);
    m_presentation.reset(new Q3DSUipPresentation);
    m_viewportSettings.reset(new Q3DSViewportSettings);

    m_asyncRenderTimer.setInterval(20);
    connect(&m_asyncRenderTimer, &QTimer::timeout, [this]() {
        if (m_widget)
            m_widget->update();
    });
}

Q3DStudioRenderer::~Q3DStudioRenderer()
{
    m_dispatch.RemoveDataModelListener(this);
    m_dispatch.RemovePresentationChangeListener(this);
    if (!m_sceneCameraMode) {
        m_dispatch.RemoveSceneDragListener(this);
        m_dispatch.RemoveToolbarChangeListener(this);
    }
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
    point.setX(int(pt.x() * m_parentPixelRatio));
    point.setY(int(pt.y() * m_parentPixelRatio));
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
    // There is a couple of frames delay after any changes to properties to actually render them on
    // screen, and there needs to be some time between the frames (possibly due to vsync), so treat
    // any render request as multiple requests. The renderNow function triggers further updates
    // asynchronously and with small delay.
    // The async updates continue for a second to ensure all pending Qt3D operations finish.
    // TODO: This is a hacky way to ensure everything gets drawn, and unnecessary renders further
    // TODO: slow down the Qt3D processing.
    // TODO: Runtime should have some signal to notify everything is now ready to be drawn.
    static const int count = 1000 / m_asyncRenderTimer.interval();
    if (m_widget && m_renderRequested < count) {
        m_asyncRenderTimer.stop();
        m_widget->update();
        m_renderRequested = count;
    }
}

bool Q3DStudioRenderer::IsInitialized()
{
    return m_widget != nullptr;
}

void Q3DStudioRenderer::initialize(QOpenGLWidget *widget, bool hasPresentation)
{
    m_widget = widget;
    m_hasPresentation = hasPresentation;

    if (m_widget && m_translation.isNull() && m_hasPresentation)
        initEngineAndTranslation();
}

void Q3DStudioRenderer::SetViewRect(const QRect &inRect, const QSize &size)
{
    if (m_widget)
        m_parentPixelRatio = StudioUtils::devicePixelRatio(m_widget->window()->windowHandle());

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
    if (!m_translation || !editCameraEnabled())
        return;

    m_translation->editCameraZoomToFit();

    RequestRender();
}

bool Q3DStudioRenderer::isMouseDown() const
{
    return m_mouseDown || (!m_scenePicker.isNull()
                           && (m_scenePicker->state() == Q3DSScenePicker::Triggered
                               || m_scenePicker->state() == Q3DSScenePicker::Queued));
}

void Q3DStudioRenderer::Close()
{
    OnClosingPresentation();
    m_widget = nullptr;
}

static void drawTopBottomTickMarks(QPainter &painter, int posX, int innerBottom,
                                   int innerTop, int lineHeight)
{
    painter.drawLine(QLine(posX, innerBottom - lineHeight, posX, innerBottom));
    painter.drawLine(QLine(posX, innerTop, posX, innerTop + lineHeight));
}

static void drawLeftRightTickMarks(QPainter &painter, int inYPos, int innerLeft,
                                   int innerRight, int lineLength)
{
    painter.drawLine(QLine(innerLeft - lineLength, inYPos, innerLeft, inYPos));
    painter.drawLine(QLine(innerRight, inYPos, innerRight + lineLength, inYPos));
}

void Q3DStudioRenderer::drawTickMarksOnHorizontalRects(QPainter &painter, int innerLeft,
                                                       int innerRight, int innerBottom,
                                                       int innerTop)
{
    int length = m_parentPixelRatio * CStudioPreferences::rulerSize() / 32;
    int centerPosX = innerLeft + (innerRight - innerLeft) / 2;
    drawTopBottomTickMarks(painter, centerPosX, innerBottom, innerTop, 15 * length);
    int tickInterval = rulerTickInterval * m_parentPixelRatio;
    int largeTickInterval = 10 * tickInterval;
    int mediumTickInterval = 2 * tickInterval;
    for (int incrementor = tickInterval;
         (centerPosX + incrementor) < innerRight && (centerPosX - incrementor) > innerLeft;
         incrementor += tickInterval) {
        int rightEdge = centerPosX + incrementor;
        int leftEdge = centerPosX - incrementor;
        int lineHeight = 0;
        if (incrementor % largeTickInterval == 0)
            lineHeight = 11 * length;
        else if (incrementor % mediumTickInterval)
            lineHeight = 4 * length;
        else
            lineHeight = 2 * length;

        if (rightEdge < innerRight)
            drawTopBottomTickMarks(painter, rightEdge, innerBottom, innerTop, lineHeight);
        if (leftEdge > innerLeft)
            drawTopBottomTickMarks(painter, leftEdge, innerBottom, innerTop, lineHeight);
    }
}

void Q3DStudioRenderer::drawTickMarksOnVerticalRects(QPainter &painter, int innerLeft,
                                                     int innerRight, int innerBottom,
                                                     int innerTop)
{
    int length = m_parentPixelRatio * CStudioPreferences::rulerSize() / 32;
    qreal centerPosY = innerBottom + (innerTop - innerBottom) / 2;
    drawLeftRightTickMarks(painter, centerPosY, innerLeft, innerRight, 15 * length);
    int tickInterval = rulerTickInterval * m_parentPixelRatio;
    int largeTickInterval = 10 * tickInterval;
    int mediumTickInterval = 2 * tickInterval;
    for (int incrementor = tickInterval;
         (centerPosY + incrementor) < innerTop && (centerPosY - incrementor) > innerBottom;
         incrementor += tickInterval) {
        int topEdge = centerPosY + incrementor;
        int bottomEdge = centerPosY - incrementor;
        int lineHeight = 0;
        if (incrementor % largeTickInterval == 0)
            lineHeight = 11 * length;
        else if (incrementor % mediumTickInterval)
            lineHeight = 4 * length;
        else
            lineHeight = 2 * length;

        if (topEdge < innerTop)
            drawLeftRightTickMarks(painter, topEdge, innerLeft, innerRight, lineHeight);
        if (bottomEdge > innerBottom)
            drawLeftRightTickMarks(painter, bottomEdge, innerLeft, innerRight, lineHeight);
    }
}

void Q3DStudioRenderer::performGuideDrag(qt3dsdm::Qt3DSDMGuideHandle guide,
                                         const QPoint &mousePoint)
{
    auto snapToTicks = [this](int presPoint, int max) -> int {
        // Guides snap to ruler ticks. Ruler ticks flow from center of presentation to the edges.
        int half = max / 2;
        int value = presPoint - half;
        value = rulerTickInterval * (value / rulerTickInterval);
        value += half;
        return value;
    };

    // We need the guide position in presentation coordinates
    QPoint presPoint = presentationPoint(mousePoint);

    qt3dsdm::SGuideInfo info = m_doc.GetDocumentReader().GetGuideInfo(guide);
    switch (info.m_Direction) {
    case qt3dsdm::GuideDirections::Horizontal:
        info.m_Position = snapToTicks(presPoint.y(), m_size.height() / m_parentPixelRatio);
        break;
    case qt3dsdm::GuideDirections::Vertical:
        info.m_Position = snapToTicks(presPoint.x(), m_size.width() / m_parentPixelRatio);
        break;
    default:
        break;
    }
    m_updatableEditor.EnsureEditor(QObject::tr("Drag Guide"), __FILE__, __LINE__)
            .UpdateGuide(guide, info);
    m_updatableEditor.FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle());
}

void Q3DStudioRenderer::checkGuideInPresentationRect(qt3dsdm::Qt3DSDMGuideHandle guide)
{
    qt3dsdm::SGuideInfo info = m_doc.GetDocumentReader().GetGuideInfo(guide);
    bool inPresentation = false;
    switch (info.m_Direction) {
    case qt3dsdm::GuideDirections::Horizontal:
        inPresentation = 0.0f <= info.m_Position
                && float(m_size.height() / m_parentPixelRatio) >= info.m_Position;
        break;
    case qt3dsdm::GuideDirections::Vertical:
        inPresentation = 0.0f <= info.m_Position
                && float(m_size.width() / m_parentPixelRatio) >= info.m_Position;
        break;
    default:
        break;
    }

    if (!inPresentation) {
        m_updatableEditor.EnsureEditor(QObject::tr("Delete Guide"), __FILE__, __LINE__)
                .DeleteGuide(guide);
    }
}

void Q3DStudioRenderer::drawRulersAndGuides(QPainter *painter)
{
    if (!m_guidesEnabled || editCameraEnabled() || g_StudioApp.IsAuthorZoom())
        return;

    const int pixelRatio = int(m_parentPixelRatio);
    const int offset = pixelRatio * (CStudioPreferences::rulerSize() / 2);

    const int innerLeft = offset;
    const int innerRight = m_viewRect.width() + offset;
    const int innerBottom = m_viewRect.height() + offset;
    const int innerTop = offset;

    const int outerLeft = innerLeft - offset;
    const int outerRight = innerRight + offset;
    const int outerBottom = innerBottom + offset;
    const int outerTop = innerTop - offset;

    // Draw rulers around the presentation
    painter->fillRect(QRect(outerLeft, outerTop,
                           innerLeft - outerLeft, outerBottom - outerTop),
                     m_rulerColor);
    painter->fillRect(QRect(innerRight, outerTop,
                           outerRight - innerRight, outerBottom - outerTop),
                     m_rulerColor);
    painter->fillRect(QRect(innerLeft, innerBottom,
                           innerRight - innerLeft, outerBottom - innerBottom),
                     m_rulerColor);
    painter->fillRect(QRect(innerLeft, outerTop,
                           innerRight - innerLeft, innerTop - outerTop),
                     m_rulerColor);

    // Draw tick marks on rulers
    painter->setPen(QPen(m_rulerTickColor, pixelRatio, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    drawTickMarksOnHorizontalRects(*painter, innerLeft, innerRight, innerTop, innerBottom);
    drawTickMarksOnVerticalRects(*painter, innerLeft, innerRight, innerTop, innerBottom);

    // Draw guides
    if (m_hasPresentation) {
        qt3dsdm::TGuideHandleList guides = m_doc.GetDocumentReader().GetGuides();
        qt3dsdm::Qt3DSDMGuideHandle selectedGuide;
        Q3DStudio::SSelectedValue selection = m_doc.GetSelectedValue();
        if (selection.getType() == Q3DStudio::SelectedValueTypes::Guide)
            selectedGuide = selection.getData<qt3dsdm::Qt3DSDMGuideHandle>();

        const QRect presRect(offset, offset, m_size.width(), m_size.height());

        // Depending on presentation dimensions and screen pixel ratio, guides may draw one pixel
        // off in some direction. Define some offsets to get pixel perfect line drawing with
        // every combination of odd/even presentation dimensions and 1x/2x pixel ratios.
        const int pixelRatioOffset = pixelRatio / 2;
        const int guideLineBottomOffset = 1;
        const int guideLineRightOffset = 1;
        const int guideLineHorizontalOffset = 1 - ((m_size.height() / pixelRatio) % 2);
        const int guideLineVerticalOffset = pixelRatioOffset * ((m_size.height() / pixelRatio) % 2);

        QPen guideSelectedPen(m_guideSelectedColor, pixelRatio, Qt::SolidLine, Qt::FlatCap,
                              Qt::MiterJoin);
        QPen guidePen(m_guideColor, pixelRatio, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);

        for (size_t guideIdx = 0, guideEnd = guides.size(); guideIdx < guideEnd; ++guideIdx) {
            qt3dsdm::SGuideInfo info = m_doc.GetDocumentReader().GetGuideInfo(guides[guideIdx]);
            bool isGuideSelected = guides[guideIdx] == selectedGuide;
            int halfWidth = pixelRatio * ((info.m_Width + 1) / 2);
            QRect guideRect = presRect;
            guideRect.adjust(1, pixelRatioOffset, -pixelRatioOffset, -1);
            painter->setPen(isGuideSelected ? guideSelectedPen : guidePen);
            switch (info.m_Direction) {
            case qt3dsdm::GuideDirections::Horizontal: {
                const int guidePos = presRect.bottom() - info.m_Position * pixelRatio
                        + guideLineHorizontalOffset;
                if (info.m_Width <= 1) {
                    painter->drawLine(presRect.left(), guidePos,
                                      presRect.right() + guideLineRightOffset,
                                      guidePos);
                } else {
                    guideRect.setTop(guidePos + halfWidth);
                    guideRect.setBottom(guidePos - halfWidth);
                }
                break;
            }
            case qt3dsdm::GuideDirections::Vertical: {
                const int guidePos = offset + info.m_Position * pixelRatio
                        + guideLineVerticalOffset;
                if (info.m_Width <= 1) {
                    painter->drawLine(guidePos, presRect.top(),
                                      guidePos, presRect.bottom() + guideLineBottomOffset);
                } else {
                    guideRect.setLeft(guidePos + halfWidth);
                    guideRect.setRight(guidePos - halfWidth);
                }
                break;
            }
            default:
                break;
            }

            // Guides with width one have already been drawn as lines
            if (info.m_Width > 1) {
                // Only fill guide rects if they need filling (on hdpi width 2 already needs fill)
                painter->fillRect(guideRect, isGuideSelected ? m_guideSelectedFillColor
                                                             : m_guideFillColor);
                painter->drawRect(guideRect);
            }
        }
    }
}

void Q3DStudioRenderer::renderNow()
{
    // TODO: Not needed until QT3DS-2072 is implemented.
    // TODO: Subpresentations need to be registered for scene camera preview renderer, too.
//    if (m_setSubpresentationsCalled == false)
//        return;

    initialize(m_widget, m_hasPresentation);

    QOpenGLContextPrivate *ctxD = QOpenGLContextPrivate::get(m_widget->context());
    QScopedValueRollback<GLuint> defaultFboRedirectRollback(ctxD->defaultFboRedirect, 0);

    if (!m_translation.isNull()) {
        m_translation->prepareRender(QRect(0, 0, m_size.width(), m_size.height()), m_size,
                                     m_parentPixelRatio);

        // m_translation->prepareRender clears context, so make sure to activate it again
        if (!QOpenGLContext::currentContext())
            m_widget->makeCurrent();

        auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                    Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        renderAspectD->renderSynchronous(true);
    }

    if (--m_renderRequested > 0) {
        if (!m_asyncRenderTimer.isActive())
            m_asyncRenderTimer.start();
    } else {
        m_asyncRenderTimer.stop();
    }
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

        // Exit the aspect engine loop before shutdown to avoid random crashes
        if (m_engine->aspectEngine())
            Qt3DCore::QAspectEnginePrivate::get(m_engine->aspectEngine())->exitSimulationLoop();

        auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                    Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        renderAspectD->renderShutdown();
        m_widget->doneCurrent();
        m_renderAspect = nullptr;

        // This will destroy render aspect
        m_translation.reset();
        m_engine->setPresentation(nullptr);
        m_engine.reset(new Q3DSEngine);
        m_presentation.reset(new Q3DSUipPresentation);
        m_viewportSettings.reset(new Q3DSViewportSettings);
    }
    m_hasPresentation = false;
    m_resizeToQt3DSent = false;
}

PickTargetAreas Q3DStudioRenderer::getPickArea(const QPoint &point)
{
    const int pickPointX(point.x());
    const int pickPointY(point.y());

    if (pickPointX < m_viewRect.left() || pickPointX > m_viewRect.right()
        || pickPointY < m_viewRect.top() || pickPointY > m_viewRect.bottom()) {
        return PickTargetAreas::Matte;
    }
    return PickTargetAreas::Presentation;
}

QPoint Q3DStudioRenderer::scenePoint(const QPoint &viewPoint)
{
    return QPoint(viewPoint.x() - m_viewRect.left(),
                  viewPoint.y() - m_viewRect.top());
}

QPoint Q3DStudioRenderer::presentationPoint(const QPoint &viewPoint)
{
    QPoint presPoint = scenePoint(viewPoint);
    presPoint.setY(m_size.height() - presPoint.y());
    return presPoint / m_parentPixelRatio;
}

qt3ds::foundation::Option<qt3dsdm::SGuideInfo> Q3DStudioRenderer::pickRulers(
        const QPoint &inMouseCoords)
{
    if (m_guidesEnabled && !editCameraEnabled()) {
        // Rulers are drawn around the m_viewRect at constant thickness
        int thickness = m_parentPixelRatio * (CStudioPreferences::rulerSize() / 2);
        QRect outerRect = m_viewRect.adjusted(-thickness, -thickness, thickness, thickness);
        if (outerRect.contains(inMouseCoords)
                && !m_viewRect.contains(inMouseCoords)) {
            std::shared_ptr<qt3dsdm::IGuideSystem> theGuideSystem =
                    g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetFullSystem()
                    ->GetCoreSystem()->GetGuideSystem();
            if (inMouseCoords.x() >= m_viewRect.left() && inMouseCoords.x() <= m_viewRect.right()) {
                return qt3dsdm::SGuideInfo(inMouseCoords.y() - m_viewRect.top(),
                                           qt3dsdm::GuideDirections::Horizontal);
            } else if (inMouseCoords.y() <= m_viewRect.bottom()
                       && inMouseCoords.y() >= m_viewRect.top()) {
                return qt3dsdm::SGuideInfo(inMouseCoords.x() - m_viewRect.left(),
                                           qt3dsdm::GuideDirections::Vertical);
            }
        }
    }

    return qt3ds::foundation::Option<qt3dsdm::SGuideInfo>();
}

SStudioPickValue Q3DStudioRenderer::pick(const QPoint &inMouseCoords, SelectMode inSelectMode,
                                         bool objectPick)
{
    QPoint renderPoint = scenePoint(inMouseCoords);

    if (!objectPick && m_doc.GetDocumentReader().AreGuidesEditable()) {
        qt3dsdm::TGuideHandleList theGuides = m_doc.GetDocumentReader().GetGuides();
        QPoint presPoint = presentationPoint(inMouseCoords);
        for (size_t guideIdx = 0, guideEnd = theGuides.size(); guideIdx < guideEnd; ++guideIdx) {
            qt3dsdm::SGuideInfo theGuideInfo =
                m_doc.GetDocumentReader().GetGuideInfo(theGuides[guideIdx]);
            float width = (theGuideInfo.m_Width / 2.0f) + (2.0f * m_parentPixelRatio);
            switch (theGuideInfo.m_Direction) {
            case qt3dsdm::GuideDirections::Horizontal:
                if (qAbs(float(presPoint.y()) - theGuideInfo.m_Position) <= width)
                    return theGuides[guideIdx];
                break;
            case qt3dsdm::GuideDirections::Vertical:
                if (qAbs(float(presPoint.x()) - theGuideInfo.m_Position) <= width)
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
    if (!objectPick && m_LastRenderedWidget && (m_LastRenderedWidget->GetNode().m_Flags.IsActive()
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
        bool retry = true;
        while (retry) {
            ensurePicker();
            retry = false;
            if (m_scenePicker->state() == Q3DSScenePicker::Unqueued) {
                m_objectPicking = objectPick;
                m_scenePicker->pick(renderPoint);
                if (m_scenePicker->state() == Q3DSScenePicker::Failed) {
                    m_scenePicker->reset();
                    return SStudioPickValue();
                } else {
                    return SStudioPickValue(StudioPickValueTypes::Pending);
                }
            } else if (!objectPick) {
                // When picking for drag directly from the scene and there is a pick pending,
                // we need to create a new scene picker and make sure the previous pick result is
                // ignored, as a pending picker cannot be reused until pick is finalized.
                // We can't delete a pending picker yet, as runtime still holds a pointer to it
                m_discardedPickers.insert(m_scenePicker.take());
                retry = true;
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
    // Ignore results on obsolete picks
    auto picker = qobject_cast<Q3DSScenePicker *>(sender());
    if (picker != m_scenePicker.data()) {
        m_discardedPickers.remove(picker);
        picker->deleteLater();
        return;
    }

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
    Q3DSGraphObject *object = m_scenePicker->pickResults().first().m_object;
    Q3DSGraphObjectTranslator *translator = Q3DSGraphObjectTranslator::translatorForObject(object);

#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    // check hit distance to cameras and lights
    if (lastIndex != -1 && thePickResult.m_CameraDistanceSq > lastDist * lastDist) {
        DoPrepareForDrag(static_cast<SNode *>(
                             &(m_editModeCamerasAndLights[lastIndex]->GetGraphObject())));
        return m_editModeCamerasAndLights[lastIndex]->GetInstanceHandle();
    }
#endif

    if (!translator) {
        if (objectPick)
            return SStudioPickValue();
        m_translation->prepareWidgetDrag(scenePoint(m_mouseDownPoint), object);
        return SWidgetPick(0);
    }

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
            m_translation->prepareDrag(scenePoint(m_mouseDownPoint), translator);
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
        // Proper pick result will be obtained asynchronously
        return;
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

    inPoint.setX(int(inPoint.x() * m_parentPixelRatio));
    inPoint.setY(int(inPoint.y() * m_parentPixelRatio));

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

            SStudioPickValue pickResult = pick(inPoint, theSelectMode, false);
            handlePickResult(pickResult, false);
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

    inPoint.setX(int(inPoint.x() * m_parentPixelRatio));
    inPoint.setY(int(inPoint.y() * m_parentPixelRatio));

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
        || m_dragPickResult.getType() == StudioPickValueTypes::Widget
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
                        const QPoint renderPoint = scenePoint(inPoint);
                        const QPoint mouseDownRenderPoint = scenePoint(m_mouseDownPoint);

                        switch (theMovement) {
                        case MovementTypes::TranslateAlongCameraDirection:
                            m_translation->translateAlongCameraDirection(
                                        mouseDownRenderPoint, renderPoint, m_updatableEditor);
                            break;
                        case MovementTypes::Translate:
                            m_translation->translate(renderPoint, m_updatableEditor, theLockToAxis);
                            break;
                        case MovementTypes::ScaleZ:
                            m_translation->scaleZ(mouseDownRenderPoint, renderPoint,
                                                  m_updatableEditor);
                            break;
                        case MovementTypes::Scale:
                            m_translation->scale(mouseDownRenderPoint, renderPoint,
                                                 m_updatableEditor);
                            break;
                        case MovementTypes::Rotation:
                            m_translation->rotate(mouseDownRenderPoint, renderPoint,
                                                  m_updatableEditor, theLockToAxis);
                            break;
                        case MovementTypes::RotationAboutCameraDirection:
                            m_translation->rotateAboutCameraDirectionVector(mouseDownRenderPoint,
                                                renderPoint, m_updatableEditor);
                            break;
                        default:
                            Q_ASSERT_X(false, __FUNCTION__, "invalid movement mode");
                            break;
                        }
                        m_translation->updateWidgetProperties();
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
    }

    if (m_dragPickResult.getType() == StudioPickValueTypes::Guide)
        performGuideDrag(m_dragPickResult.getData<qt3dsdm::Qt3DSDMGuideHandle>(), inPoint);

    // if ( m_PickResult.m_WidgetId.hasValue() == false )
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
    // We need to do widget-specific dragging.
    else if (m_pickResult.getType() == StudioPickValueTypes::Widget) {
        m_Translation->PerformWidgetDrag(m_PickResult.GetWidgetId(), m_MouseDownPoint,
                                         m_PreviousMousePoint, inPoint, m_UpdatableEditor);
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
        checkGuideInPresentationRect(theSelectedGuide);
    }
    if (m_lastDragToolMode != MovementTypes::Unknown)
        m_translation->endDrag(false, m_updatableEditor);
    else
        m_translation->endPickWidget();
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
        qreal theMultiplier = 1.0 - inDelta / qreal(120 * wheelZoomFactor);
        m_translation->wheelZoom(theMultiplier);
        RequestRender();
    }
}

void Q3DStudioRenderer::OnToolbarChange()
{

}

void Q3DStudioRenderer::onSelectionChange(Q3DStudio::SSelectedValue selected)
{
    if (m_translation.isNull())
        return;

    qt3dsdm::TInstanceHandleList instances = selected.GetSelectedInstances();
    if (!instances.empty()) {
        for (auto &instance : instances) {
            if (g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->IsInstance(instance))
                m_translation->selectObject(instance);
        }
    } else {
        m_translation->unselectObject();
    }

    RequestRender();
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

    if (m_editCameraIndex != m_pendingEditCameraIndex && !m_sceneCameraMode)
        SetEditCamera(m_pendingEditCameraIndex);

    qDeleteAll(m_discardedPickers);
    m_discardedPickers.clear();
}

void Q3DStudioRenderer::createTranslation()
{
    m_translation.reset(new Q3DSTranslation(*this, m_presentation, m_sceneCameraMode));
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

std::shared_ptr<IStudioRenderer> IStudioRenderer::CreateStudioRenderer(bool sceneCameraMode)
{
    return std::shared_ptr<IStudioRenderer>(new Q3DStudioRenderer(sceneCameraMode));
}

}
