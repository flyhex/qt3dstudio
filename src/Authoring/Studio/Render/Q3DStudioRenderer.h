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
#include "DispatchListeners.h"
#include "Core.h"
#include "Dispatch.h"
#include "Q3DSEditCamera.h"
#include "StudioPickValues.h"
#include "Qt3DSDMGuides.h"
#include "IDocumentEditor.h"
#include "StudioEnums.h"

#include <q3dsruntime2api_p.h>
#include <QtWidgets/qopenglwidget.h>

QT_BEGIN_NAMESPACE
namespace Qt3DRender {
class QRenderAspect;
}
QT_END_NAMESPACE

namespace Q3DStudio {

class Q3DSTranslation;
class Q3DStudioRenderer : public QObject,
                          public IStudioRenderer,
                          public IDataModelListener,
                          public IReloadListener,
                          public CPresentationChangeListener,
                          public CSceneDragListener,
                          public CToolbarChangeListener
{
public:
    Q3DStudioRenderer();
    ~Q3DStudioRenderer() override;
    ITextRenderer *GetTextRenderer() override;
    QT3DSVec3 GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inHandle, CPt inPoint) override;
    Q3DSRenderBufferManager *GetBufferManager() override;
    IPathManager *GetPathManager() override;
    qt3ds::foundation::IStringTable *GetRenderStringTable() override;
    void RequestRender() override;
    bool IsInitialized() override;
    void Initialize(QWidget *inWindow) override;
    void SetViewRect(const QRect &inRect, const QSize &size) override;
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

    QSharedPointer<Q3DSEngine> &engine();

    QRect viewRect() const
    {
        return m_viewRect;
    }
protected:
    void OnBeginDataModelNotifications() override;
    void OnEndDataModelNotifications() override;
    void OnImmediateRefreshInstanceSingle(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnImmediateRefreshInstanceMultiple(qt3dsdm::Qt3DSDMInstanceHandle *inInstance,
                                                    long inInstanceCount) override;
    void OnReloadEffectInstance(qt3dsdm::Qt3DSDMInstanceHandle inInstance) override;
    void OnNewPresentation() override;
    void OnClosingPresentation() override;
    void OnSceneMouseDown(SceneDragSenderType::Enum inSenderType, QPoint inPoint, int) override;
    void OnSceneMouseDrag(SceneDragSenderType::Enum, QPoint inPoint, int inToolMode,
                          int inFlags) override;
    void OnSceneMouseUp(SceneDragSenderType::Enum) override;
    void OnSceneMouseDblClick(SceneDragSenderType::Enum inSenderType, QPoint inPoint) override;
    void OnSceneMouseWheel(SceneDragSenderType::Enum inSenderType, short inDelta,
                           int inToolMode) override;
    void OnToolbarChange() override;
    void OnSelectionChange();
    void onScenePick();

private:

    bool editCameraEnabled() const
    {
        return m_editCameraIndex > -1 ? true : false;
    }
    void createEngine();
    void createTranslation();
    void sendResizeToQt3D(const QSize &size);
    qreal fixedDevicePixelRatio() const;
    void drawGuides(QPainter &painter);

    void drawTickMarksOnHorizontalRects(QPainter &painter, qreal innerLeft,
                                        qreal innerRight, qreal innerBottom, qreal innerTop,
                                        qreal outerBottom, qreal outerTop);
    void drawTickMarksOnVerticalRects(QPainter &painter, qreal innerLeft,
                                      qreal innerRight, qreal innerBottom, qreal innerTop,
                                      qreal outerLeft, qreal outerRight);

    PickTargetAreas getPickArea(const QPoint &point);
    QPoint scenePoint(const QPoint &viewPoint);
    qt3ds::foundation::Option<qt3dsdm::SGuideInfo> pickRulers(CPt inMouseCoords);
    SStudioPickValue pick(const QPoint &inMouseCoords, SelectMode inSelectMode);
    void handlePickResult();
    SStudioPickValue postScenePick();

    CDispatch &m_dispatch;
    CDoc &m_doc;
    qt3dsdm::TSignalConnectionPtr m_selectionSignal;
    QSharedPointer<Q3DSEngine> m_engine;
    QOpenGLWidget *m_widget = nullptr;
    Qt3DRender::QRenderAspect *m_renderAspect = nullptr;
    Q3DSViewportSettings m_viewportSettings;
    QScopedPointer<Q3DSTranslation> m_translation;
    QVector<SEditCameraPersistentInformation> m_editCameraInformation;
    QRect m_viewRect;
    QSize m_size;
    QRect m_innerRect;
    QRect m_outerRect;
    QColor m_rectColor;
    QColor m_lineColor;
    bool m_guidesEnabled = true;
    bool m_hasPresentation = false;
    bool m_renderRequested = false;
    int m_editCameraIndex = -1;
    SStudioPickValue m_pickResult;
    CUpdateableDocumentEditor m_updatableEditor;
    QPoint m_mouseDownPoint;
    QPoint m_previousMousePoint;
    MovementTypes m_lastDragToolMode = MovementTypes::Unknown;
    bool m_maybeDragStart = false;
    SEditCameraPersistentInformation m_mouseDownCameraInformation;
    int m_lastToolMode = 0;
    QScopedPointer<Q3DSScenePicker> m_scenePicker;
    bool m_pickPending = false;
};

}

#endif
