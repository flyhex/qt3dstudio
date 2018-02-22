/****************************************************************************
**
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

#include "TimelineView.h"
#include "Core.h"
#include "Doc.h"
#include "Dispatch.h"
#include "Bindings/TimelineTranslationManager.h"
#include "HotKeys.h"

#include "Literals.h"
#include "StudioApp.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"

#include "TimeMeasureItem.h"
#include "TimePropertyItem.h"

#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMHandles.h"

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtCore/qtimer.h>
#include <QtWidgets/qaction.h>

namespace {
const qreal SCALING_PERCENTAGE_INC = 1.1;
const qreal SCALING_PERCENTAGE_DEC = 0.9;
}

TimelineView::TimelineView(QWidget *parent) : QQuickWidget(parent)
{
    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);
    g_StudioApp.GetCore()->GetDispatch()->AddClientPlayChangeListener(this);
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_translationManager = new CTimelineTranslationManager();

    QTimer::singleShot(0, this, &TimelineView::initialize);
}

QAbstractItemModel *TimelineView::objectModel() const
{
    return m_model; // should be safe as it is passed only to QML
}

void TimelineView::setSelection(int index)
{
    if (m_selection != index) {
        m_selection = index;
        Q_EMIT selectionChanged();
    }
}

void TimelineView::setTimeRatio(qreal timeRatio)
{
    timeRatio = qMin(timeRatio, 1.0);

    if (!qFuzzyCompare(m_timeRatio, timeRatio)) {
        m_timeRatio = timeRatio;

        for (int i = 0; i < m_model->rowCount(); ++i) {
            auto timelineRow = m_model->index(i, 0)
                .data(TimelineObjectModel::TimelineRowRole).value<CTimelineRow*>();
            timelineRow->SetTimeRatio(timeRatio);
        }

        Q_EMIT timeRatioChanged(timeRatio);
    }
}

void TimelineView::OnNewPresentation()
{
    m_translationManager->OnNewPresentation();

    // Register callbacks
    qt3dsdm::IStudioFullSystemSignalProvider *theSignalProvider =
        GetDoc()->GetStudioSystem()->GetFullSystemSignalProvider();

    m_Connections.push_back(theSignalProvider->ConnectActiveSlide(
        std::bind(&TimelineView::OnActiveSlide, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3)));

    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    m_Connections.push_back(theDispatch->ConnectSelectionChange(
        std::bind(&TimelineView::OnSelectionChange, this, std::placeholders::_1)));

    m_Connections.push_back(theSignalProvider->ConnectAnimationCreated(
        std::bind(&TimelineView::OnAnimationCreated, this,
             std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectAnimationDeleted(
        std::bind(&TimelineView::OnAnimationDeleted, this,
             std::placeholders::_2, std::placeholders::_3)));

    m_Connections.push_back(theSignalProvider->ConnectActionCreated(
        std::bind(&TimelineView::OnActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectActionDeleted(
        std::bind(&TimelineView::OnActionEvent, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));
}

void TimelineView::OnClosingPresentation()
{
    m_Connections.clear();
}

void TimelineView::OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable)
{
    // Expand the tree so the selection is visible
    qt3dsdm::TInstanceHandleList theInstances = inNewSelectable.GetSelectedInstances();
    for (size_t idx = 0, end = theInstances.size(); idx < end; ++idx) {
        qt3dsdm::Qt3DSDMInstanceHandle theInstance(theInstances[idx]);
        if (GetDoc()->GetStudioSystem()->IsInstance(theInstance)) {
            auto index = m_model->sourceIndexForHandle(theInstance);
            m_model->expandTo(QModelIndex(), index);
        }
    }
    m_translationManager->OnSelectionChange(inNewSelectable);
}

void TimelineView::OnTimeChanged(long inTime)
{
    setCurrentTime(inTime);
}

void TimelineView::select(int index, Qt::KeyboardModifiers modifiers)
{
    auto timelineRow = m_model->index(index, 0)
            .data(TimelineObjectModel::TimelineRowRole).value<CTimelineRow*>();
    timelineRow->Select(modifiers);
    setSelection(index);
}

void TimelineView::setNewTimePosition(double position)
{
    auto time = ::PosToTime(position, m_timeRatio);
    GetDoc()->NotifyTimeChanged(time);
}

void TimelineView::setHideShy(bool enabled)
{
    if (m_objectListModel && m_objectListModel->hideShy() != enabled) {
        m_objectListModel->setHideShy(enabled);
        emit hideShyChanged();
    }
}

bool TimelineView::hideShy() const
{
    if (m_objectListModel)
        return m_objectListModel->hideShy();

    return false;
}

void TimelineView::setHideHidden(bool enabled)
{
    if (m_objectListModel && m_objectListModel->hideHidden() != enabled) {
        m_objectListModel->setHideHidden(enabled);
        emit hideHiddenChanged();
    }
}

bool TimelineView::hideHidden() const
{
    if (m_objectListModel)
        return m_objectListModel->hideHidden();

    return false;
}

void TimelineView::setHideLocked(bool enabled)
{
    if (m_objectListModel && m_objectListModel->hideLocked() != enabled) {
        m_objectListModel->setHideLocked(enabled);
        emit hideLockedChanged();
    }
}

bool TimelineView::hideLocked() const
{
    if (m_objectListModel)
        return m_objectListModel->hideLocked();

    return false;
}

void TimelineView::setCurrentTime(long newTime)
{
    if (m_currentTime != newTime) {
        m_currentTime = newTime;

        Q_EMIT currentTimeChanged();
    }
}

long TimelineView::currentTime() const
{
    return m_currentTime;
}

double TimelineView::currentTimePos() const
{
    return ::TimeToPos(m_currentTime, m_timeRatio);
}

void TimelineView::OnAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                      qt3dsdm::Qt3DSDMPropertyHandle property)
{
    if (m_objectListModel)
        m_objectListModel->addProperty(parentInstance, property);
}

void TimelineView::OnActionEvent(qt3dsdm::Qt3DSDMActionHandle inAction,
                                 qt3dsdm::Qt3DSDMSlideHandle inSlide,
                                 qt3dsdm::Qt3DSDMInstanceHandle inOwner)
{
    Q_UNUSED(inAction);
    Q_UNUSED(inSlide);
    Q_UNUSED(inOwner);
    if (m_objectListModel) {
        m_objectListModel->updateActions();
    }
}

void TimelineView::OnAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                      qt3dsdm::Qt3DSDMPropertyHandle property)
{
    if (m_objectListModel)
        m_objectListModel->removeProperty(parentInstance, property);
}

void TimelineView::OnActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex,
                                 const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    Q_UNUSED(inMaster);
    Q_UNUSED(inIndex);

    if (m_activeSlide == inSlide)
        return;

    m_translationManager->Clear();
    m_activeSlide = inSlide;
    qDebug() << "OnActiveSlide";
    auto *theSlideSystem = GetDoc()->GetStudioSystem()->GetSlideSystem();
    auto theSlideInstance = theSlideSystem->GetSlideInstance(inSlide);

    m_objectListModel = new TimelineObjectModel(g_StudioApp.GetCore(),
                                                GetDoc()->GetActiveRootInstance(), this);
    m_objectListModel->setTimelineItemBinding(m_translationManager->GetOrCreate(theSlideInstance));
    if (!m_model)
        m_model = new FlatObjectListModel(m_objectListModel, this);
    else
        m_model->setSourceModel(m_objectListModel);

    // expand by default the first level
    m_model->expandTo(QModelIndex(), m_objectListModel->index(0, 0, m_objectListModel->index(0,0)));
    emit objectModelChanged();
}

void TimelineView::initialize()
{
    CStudioPreferences::setQmlContextProperties(rootContext());
    rootContext()->setContextProperty("_timelineView"_L1, this);
    rootContext()->setContextProperty("_resDir"_L1, resourceImageUrl());

    qmlRegisterUncreatableType<TimelineView>("Qt3DStudio", 1, 0, "TimelineView"
                                             , tr("Creation of TimelineView not allowed from QML"));
    qmlRegisterUncreatableType<TimebarTimeInfo>("Qt3DStudio", 1, 0, "TimebarTimeInfo"
                                             , tr("Creation of TimebarTimeInfo not allowed from QML"));
    qmlRegisterUncreatableType<KeyframeInfo>("Qt3DStudio", 1, 0, "KeyframeInfo"
                                             , tr("Creation of KeyframeInfo not allowed from QML"));
    qmlRegisterUncreatableType<CTimelineRow>("Qt3DStudio", 1, 0, "TimelineRow"
                                             , tr("Creation of TimelineRow not allowed from QML"));
    qmlRegisterType<TimeMeasureItem>("Qt3DStudio", 1, 0, "TimeMeasureItem");
    qmlRegisterType<TimePropertyItem>("Qt3DStudio", 1, 0, "TimePropertyItem");
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Timeline/Timeline.qml"_L1));
}

CDoc *TimelineView::GetDoc()
{
    return g_StudioApp.GetCore()->GetDoc();
}

void TimelineView::OnScalingZoomIn()
{
    const qreal timeRatio = m_timeRatio * SCALING_PERCENTAGE_INC;
    setTimeRatio(timeRatio);
}

void TimelineView::OnScalingZoomOut()
{
    const qreal timeRatio = m_timeRatio * SCALING_PERCENTAGE_DEC;
    setTimeRatio(timeRatio);
}

void TimelineView::RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler, QWidget *actionParent)
{
    Q_UNUSED(inShortcutHandler);

    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::Key_Plus),
                        TimelineView::OnScalingZoomIn);
    ADD_GLOBAL_SHORTCUT(actionParent,
                        QKeySequence(Qt::Key_Minus),
                        TimelineView::OnScalingZoomOut);
}
