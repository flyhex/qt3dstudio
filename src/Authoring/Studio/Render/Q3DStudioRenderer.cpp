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

    ::CColor color = CStudioPreferences::GetRulerBackgroundColor(); // Rectangles under tick marks
    m_rectColor = QColor(int(color.GetRed()), int(color.GetGreen()), int(color.GetBlue()));
    color = CStudioPreferences::GetRulerTickColor(); // Tick marks
    m_lineColor = QColor(int(color.GetRed()), int(color.GetGreen()), int(color.GetBlue()));
    m_editCameraInformation.resize(g_numEditCameras);
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

void Q3DStudioRenderer::SetViewRect(const QRect &inRect, const QSize &size)
{
    m_viewRect = inRect;
    m_size = size;
    if (!m_engine.isNull())
        m_engine->resize(size, fixedDevicePixelRatio(), false);
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
    if (index != m_editCameraIndex) {
        // save old edit camera info
        if (editCameraEnabled())
            m_editCameraInformation[m_editCameraIndex] = m_translation->editCameraInfo();
        m_editCameraIndex = index;
        if (index == -1) {
            // use scene camera
            m_translation->disableEditCamera();
            m_viewportSettings.setMatteEnabled(true);
        } else {
            // use edit camera
            m_viewportSettings.setMatteEnabled(false);
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

void Q3DStudioRenderer::Close()
{
    OnClosingPresentation();
    if (m_engine.data()) {
        m_engine->setPresentation(nullptr);
        m_engine.reset();
    }
    m_dispatch.RemoveDataModelListener(this);
    m_dispatch.RemovePresentationChangeListener(this);
    m_dispatch.RemoveSceneDragListener(this);
    m_dispatch.RemoveToolbarChangeListener(this);
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

void Q3DStudioRenderer::RenderNow()
{
    m_renderRequested = false;
    QOpenGLContextPrivate *ctxD = QOpenGLContextPrivate::get(m_widget->context());
    QScopedValueRollback<GLuint> defaultFboRedirectRollback(ctxD->defaultFboRedirect, 0);

    QOpenGLPaintDevice device;
    device.setSize(m_widget->size());
    QPainter painter(&device);

    if (m_engine.isNull()) {
        createEngine();

        auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                    Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        renderAspectD->renderInitialize(m_widget->context());
        m_widget->makeCurrent();
    }

    if (m_translation.isNull() && m_hasPresentation)
        createTranslation();

    if (!m_translation.isNull())
        m_translation->prepareRender(m_viewRect, m_size);

    if (!QOpenGLContext::currentContext())
        m_widget->makeCurrent();

    if (!editCameraEnabled()) {
        QColor matteColor;
        matteColor.setRgbF(.13, .13, .13);
        painter.fillRect(0, 0, m_widget->width(), m_widget->height(), matteColor);
    }
    painter.beginNativePainting();
    auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
    renderAspectD->renderSynchronous(true);
    painter.endNativePainting();

    // fix gl state leakage
    QOpenGLContext::currentContext()->functions()->glDisable(GL_STENCIL_TEST);
    QOpenGLContext::currentContext()->functions()->glDisable(GL_DEPTH_TEST);
    QOpenGLContext::currentContext()->functions()->glDisable(GL_CULL_FACE);

    // draw guides if enabled
    drawGuides(painter);
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
    if (!m_engine.isNull())
        createTranslation();
}

void Q3DStudioRenderer::OnClosingPresentation()
{
    if (!m_hasPresentation || !m_widget)
        return;
    m_widget->makeCurrent();
    if (!m_engine.isNull() && !m_translation.isNull()) {
        auto renderAspectD = static_cast<Qt3DRender::QRenderAspectPrivate *>(
                    Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        renderAspectD->renderShutdown();
        m_renderAspect = nullptr;
        m_widget->doneCurrent();
        /* This would destroy render aspect. */
        m_engine->setPresentation(nullptr);
        m_engine.reset();
        m_translation.reset();
    }
    m_hasPresentation = false;
}

PickTargetAreas Q3DStudioRenderer::getPickArea(CPt inPoint)
{
    const qreal pickPointX(inPoint.x);
    const qreal pickPointY(m_viewRect.height() - inPoint.y);
    QRectF rect = m_viewRect;
    if (editCameraEnabled()) {
        qreal offset = CStudioPreferences::guideSize() / 2;
        rect.setLeft(rect.left() + offset);
        rect.setRight(rect.left() - offset);
        rect.setTop(rect.top() + offset);
        rect.setBottom(rect.bottom() - offset);
    }
    if (pickPointX < rect.left() || pickPointX > rect.right()
        || pickPointY < rect.bottom() || pickPointY > rect.top()) {
        return PickTargetAreas::Matte;
    }
    return PickTargetAreas::Presentation;
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

void Q3DStudioRenderer::OnSceneMouseDown(SceneDragSenderType::Enum inSenderType,
                                         QPoint inPoint, int)
{
    if (m_translation.isNull())
        return;

    inPoint.setX(int(inPoint.x() * devicePixelRatio()));
    inPoint.setY(int(inPoint.y() * devicePixelRatio()));

    m_pickResult = SStudioPickValue();
    if (inSenderType == SceneDragSenderType::SceneWindow) {
        PickTargetAreas pickArea = getPickArea(inPoint);
        if (pickArea == PickTargetAreas::Presentation) {
            TranslationSelectMode theSelectMode = TranslationSelectMode::Group;
            switch (g_StudioApp.GetSelectMode()) {
            case STUDIO_SELECTMODE_ENTITY:
                theSelectMode = TranslationSelectMode::Single;
                break;
            case STUDIO_SELECTMODE_GROUP:
                theSelectMode = TranslationSelectMode::Group;
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
            // TODO: picking
        } else if (pickArea == PickTargetAreas::Matte) {
            qt3ds::foundation::Option<qt3dsdm::SGuideInfo> pickResult = pickRulers(inPoint);
            if (pickResult.hasValue()) {
                Q3DStudio::IDocumentEditor &docEditor(
                            m_updatableEditor.EnsureEditor(QObject::tr("Create Guide"),
                                                           __FILE__, __LINE__));
                qt3dsdm::Qt3DSDMGuideHandle newGuide = docEditor.CreateGuide(*pickResult);
                m_pickResult = SStudioPickValue(newGuide);
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
    if (m_translation.isNull())
        return;

    inPoint.setX(int(inPoint.x() * devicePixelRatio()));
    inPoint.setY(int(inPoint.y() * devicePixelRatio()));

    if (m_maybeDragStart) {
        QPoint theDragDistance = inPoint - m_mouseDownPoint;
        const int dragDistanceLimit = QApplication::startDragDistance();
        if (m_pickResult.getType() == StudioPickValueTypes::Widget
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
        m_pickResult = SStudioPickValue();
    m_lastToolMode = inToolMode;

    // General dragging
    if (m_pickResult.getType() == StudioPickValueTypes::Instance
        || m_pickResult.getType()
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
                        Q_ASSERT_X(false, "Q3DStudioRenderer::OnSceneMouseDrag", "unreachable code");
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
#if RUNTIME_SPLIT_TEMPORARILY_REMOVED
                        switch (theMovement) {
                        case MovementTypes::TranslateAlongCameraDirection:
                            m_Translation->TranslateSelectedInstanceAlongCameraDirection(
                                m_MouseDownPoint, inPoint, m_UpdatableEditor);
                            break;
                        case MovementTypes::Translate:
                            m_Translation->TranslateSelectedInstance(
                                m_MouseDownPoint, inPoint, m_UpdatableEditor, theLockToAxis);
                            break;
                        case MovementTypes::ScaleZ:
                            m_Translation->ScaleSelectedInstanceZ(m_MouseDownPoint, inPoint,
                                                                  m_UpdatableEditor);
                            break;
                        case MovementTypes::Scale:
                            m_Translation->ScaleSelectedInstance(m_MouseDownPoint, inPoint,
                                                                 m_UpdatableEditor);
                            break;
                        case MovementTypes::Rotation:
                            m_Translation->RotateSelectedInstance(
                                        m_MouseDownPoint, m_PreviousMousePoint, inPoint,
                                        m_UpdatableEditor, theLockToAxis);
                            break;
                        case MovementTypes::RotationAboutCameraDirection:
                            m_Translation->RotateSelectedInstanceAboutCameraDirectionVector(
                                m_PreviousMousePoint, inPoint, m_UpdatableEditor);
                            break;
                        default:
                            break;
                        }
#endif
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
    qt3dsdm::Qt3DSDMGuideHandle theSelectedGuide;
    if (m_pickResult.getType() == StudioPickValueTypes::Guide) {
        theSelectedGuide = m_pickResult.getData<qt3dsdm::Qt3DSDMGuideHandle>();
        // TODO:
        //m_Translation->CheckGuideInPresentationRect(theSelectedGuide, m_UpdatableEditor);
    }
    m_updatableEditor.CommitEditor();
    m_pickResult = SStudioPickValue();

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
    m_viewportSettings.setShowRenderStats(false);
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
