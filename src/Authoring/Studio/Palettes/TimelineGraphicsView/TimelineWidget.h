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

#ifndef TIMELINEWIDGET_H
#define TIMELINEWIDGET_H

#include <QtWidgets/qwidget.h>
#include <QtCore/qtimer.h>
#include "DispatchListeners.h"
#include "ObjectListModel.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSignals.h"
#include "SelectedValueImpl.h"
#include "TreeHeaderView.h"
#include "Bindings/Qt3DSDMTimeline.h"
#include "NavigationBar.h"
#include "Control.h"

class RowTree;
class TimelineToolbar;
class TimelineSplitter;
class TimelineGraphicsScene;
class CTimelineTranslationManager;
class Qt3DSDMTimelineItemBinding;
class CClientDataModelBridge;
class IBreadCrumbProvider;

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)

class TimelineWidget : public QWidget,
                       public CPresentationChangeListener,
                       public CClientPlayChangeListener,
                       public CControl
{
    Q_OBJECT

public:
    explicit TimelineWidget(const QSize &preferredSize, QWidget *parent = nullptr);
    ~TimelineWidget();

    QSize sizeHint() const override;

    TimelineToolbar *toolbar() const;
    QGraphicsView *viewTimelineContent() const;
    QGraphicsView *viewTreeContent() const;
    QVector<RowTree *> selectedRows() const;
    void openBarColorDialog();
    void onTimeBarColorChanged(const QColor &color);
    void setSelectedTimeBarsColor(const QColor &color, bool preview);
    void enableDnD(bool b = true);
    bool dndActive() const;
    bool blockMousePress() const;

    // Presentation Change Listener
    void OnNewPresentation() override;
    void OnClosingPresentation() override;
    void onSelectionChange(Q3DStudio::SSelectedValue inNewSelectable);

    //CClientPlayChangeListener
    void OnTimeChanged(long inTime) override;
    bool hasSelectedKeyframes() const;

    // CControl
    void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation = false) override;
    void Draw(CRenderer *inRenderer) override;
    void OnGainFocus() override;
    CDropTarget *FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags,
                                   EStudioObjectType objectType) override;
    bool OnMouseHover(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseMove(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    CPt GetPreferredSize() override;
    void SetSize(long inX, long inY) override;
    bool isFullReconstructPending() const { return m_fullReconstruct; }
    NavigationBar *navigationBar() const { return m_navigationBar; }

protected:
    // DataModel callbacks
    virtual void onActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                               const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    void onAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void onAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void onAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle property);
    void onKeyframeInserted(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                            qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe);
    void onKeyframeDeleted(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                           qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe);
    void onKeyframeUpdated(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe);
    void onFirstKeyframeDynamicSet(qt3dsdm::Qt3DSDMAnimationHandle inAnimation, bool inDynamic);
    void onAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle property);
    void onActionEvent(qt3dsdm::Qt3DSDMActionHandle inAction, qt3dsdm::Qt3DSDMSlideHandle inSlide,
                       qt3dsdm::Qt3DSDMInstanceHandle inOwner);
    void onPropertyLinked(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                          qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void onPropertyUnlinked(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void onChildAdded(int inParent, int inChild, long inIndex);
    void onChildRemoved(int inParent, int inChild, long inIndex);
    void onChildMoved(int inParent, int inChild, long inOldIndex, long inNewIndex);
    void onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    void onAsyncUpdate();

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    typedef QHash<qt3dsdm::Qt3DSDMInstanceHandle, RowTree *> THandleMap;

    Qt3DSDMTimelineItemBinding *getBindingForHandle(int handle,
                                                    Qt3DSDMTimelineItemBinding *binding) const;
    void insertToHandlesMapRecursive(Qt3DSDMTimelineItemBinding *binding);
    void insertToHandlesMap(Qt3DSDMTimelineItemBinding *binding);
    Q3DStudio::CString getPlaybackMode();
    void refreshKeyframe(qt3dsdm::Qt3DSDMAnimationHandle inAnimation,
                         qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                         ETimelineKeyframeTransaction inTransaction);
    void updateActionStates(const QSet<RowTree *> &rows);
    void setTreeWidth(int width);

    TreeHeaderView *m_viewTreeHeader = nullptr;
    QGraphicsView *m_viewTreeContent = nullptr;
    QGraphicsView *m_viewTimelineHeader = nullptr;
    QGraphicsView *m_viewTimelineContent = nullptr;
    NavigationBar *m_navigationBar = nullptr;
    TimelineToolbar *m_toolbar = nullptr;
    TimelineGraphicsScene *m_graphicsScene;
    TimelineSplitter *m_splitter = nullptr;
    CTimelineTranslationManager *m_translationManager = nullptr;
    FlatObjectListModel *m_model = nullptr;
    Qt3DSDMTimelineItemBinding *m_binding = nullptr;
    bool m_splitterPressed = false;
    QSize m_preferredSize;
    QMultiHash<qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::Qt3DSDMPropertyHandle> m_dirtyProperties;
    QHash<int, int> m_moveMap; // key: child handle, value: parent handle
    QSet<int> m_actionChanges; // key: object handle
    QMultiHash<int, int> m_keyframeChangesMap; // key: object handle, value: property handle
    QTimer m_asyncUpdateTimer;
    bool m_fullReconstruct = false;
    bool m_blockMousePress = false;
    CClientDataModelBridge *m_bridge = nullptr;
    IBreadCrumbProvider *m_BreadCrumbProvider = nullptr;

    // data model connection
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_connections;
    qt3dsdm::Qt3DSDMSlideHandle m_activeSlide;
    THandleMap m_handlesMap;
};

#endif // TIMELINEWIDGET_H
