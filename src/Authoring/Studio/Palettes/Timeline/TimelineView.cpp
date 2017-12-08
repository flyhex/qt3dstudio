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

#include "Literals.h"
#include "StudioApp.h"
#include "StudioPreferences.h"
#include "StudioUtils.h"

#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"
#include "Qt3DSDMHandles.h"

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtCore/qtimer.h>

TimelineView::TimelineView(QWidget *parent) : QQuickWidget(parent)
{
    g_StudioApp.GetCore()->GetDispatch()->AddPresentationChangeListener(this);
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_translationManager = new CTimelineTranslationManager();

    QTimer::singleShot(0, this, &TimelineView::initialize);
}

QAbstractItemModel *TimelineView::objectModel() const
{
    return m_model.data(); // should be safe as it is passed only to QML
}

void TimelineView::setSelection(int index)
{
    if (m_selection != index) {
        m_selection = index;
        Q_EMIT selectionChanged();
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

void TimelineView::select(int index, Qt::KeyboardModifiers modifiers)
{
    auto timelineRow = m_model->index(index, 0)
            .data(TimelineObjectModel::TimelineRowRole).value<CTimelineRow*>();
    timelineRow->Select(modifiers);
    setSelection(index);
}

void TimelineView::OnAnimationCreated(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                      qt3dsdm::Qt3DSDMPropertyHandle property)
{
    if (m_objectListModel)
        m_objectListModel->addProperty(parentInstance, property);
}

void TimelineView::OnAnimationDeleted(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                                      qt3dsdm::Qt3DSDMPropertyHandle property)
{
    if (m_objectListModel)
        m_objectListModel->removeProperty(parentInstance, property);
}

void TimelineView::OnActiveSlide(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inIndex, const qt3dsdm::Qt3DSDMSlideHandle &inSlide)
{
    if (m_activeSlide == inSlide)
        return;

    m_translationManager->Clear();
    m_activeSlide = inSlide;

    auto *theSlideSystem = GetDoc()->GetStudioSystem()->GetSlideSystem();
    auto theSlideInstance = theSlideSystem->GetSlideInstance(inSlide);

    m_objectListModel.reset(new TimelineObjectModel(g_StudioApp.GetCore(),
                                                    GetDoc()->GetActiveRootInstance(), this));
    m_objectListModel->setTimelineItemBinding(m_translationManager->GetOrCreate(theSlideInstance));
    if (m_model.isNull())
        m_model.reset(new FlatObjectListModel(m_objectListModel.data(), this));
    else
        m_model->setSourceModel(m_objectListModel.data());

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
    qmlRegisterUncreatableType<TimebarTimeInfo>("Qt3DStudio", 1, 0, "KeyframeInfo"
                                             , tr("Creation of KeyframeInfo not allowed from QML"));
    engine()->addImportPath(qmlImportPath());
    setSource(QUrl("qrc:/Palettes/Timeline/Timeline.qml"_L1));
}

CDoc *TimelineView::GetDoc()
{
    return g_StudioApp.GetCore()->GetDoc();
}
