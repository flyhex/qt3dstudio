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
#include "DispatchListeners.h"
#include "ObjectListModel.h"
#include "Qt3DSDMHandles.h"
#include "Qt3DSDMSignals.h"
#include "SelectedValueImpl.h"
#include "TimelineObjectModel.h"

class RowTree;
class TimelineToolbar;
class TimelineSplitter;
class TimelineGraphicsScene;
class CTimelineTranslationManager;
class Qt3DSDMTimelineItemBinding;

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QGraphicsView)

class TimelineWidget : public QWidget,
                       public CPresentationChangeListener,
                       public CClientPlayChangeListener
{
    Q_OBJECT

public:
    explicit TimelineWidget(QWidget *parent = nullptr);

    TimelineToolbar *toolbar() const;
    QGraphicsView *viewTimelineContent() const;
    QGraphicsView *viewTreeContent() const;

    // Presentation Change Listener
    void OnNewPresentation() override;
    void OnClosingPresentation() override;
    void OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable);

    //CClientPlayChangeListener
    void OnTimeChanged(long inTime) override;

protected:
    // DataModel callbacks
    virtual void OnActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                               const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    void OnAssetCreated(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void OnAssetDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void OnAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle property);
    void OnAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                            qt3dsdm::Qt3DSDMPropertyHandle property);
    void OnActionEvent(qt3dsdm::Qt3DSDMActionHandle inAction, qt3dsdm::Qt3DSDMSlideHandle inSlide,
                       qt3dsdm::Qt3DSDMInstanceHandle inOwner);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    typedef std::map<qt3dsdm::Qt3DSDMInstanceHandle, RowTree *> THandleMap;

    Qt3DSDMTimelineItemBinding *getBindingForHandle(int handle,
                                                    Qt3DSDMTimelineItemBinding *binding) const;
    void insertToHandlesMapRecursive(Qt3DSDMTimelineItemBinding *binding);
    QGraphicsView *m_viewTreeHeader = nullptr;
    QGraphicsView *m_viewTreeContent = nullptr;
    QGraphicsView *m_viewTimelineHeader = nullptr;
    QGraphicsView *m_viewTimelineContent = nullptr;
    TimelineToolbar *m_toolbar = nullptr;
    TimelineGraphicsScene *m_graphicsScene;
    TimelineSplitter *m_splitter = nullptr;
    CTimelineTranslationManager *m_translationManager = nullptr;
    TimelineObjectModel *m_objectListModel = nullptr;
    FlatObjectListModel *m_model = nullptr;
    Qt3DSDMTimelineItemBinding *m_binding;
    bool m_splitterPressed = false;

    // data model connection
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_connections;
    qt3dsdm::Qt3DSDMSlideHandle m_activeSlide;
    THandleMap m_handlesMap;
};

#endif // TIMELINEWIDGET_H
