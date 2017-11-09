/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

//==============================================================================
//	Includes
//==============================================================================
#include "PaletteManager.h"
#include "StudioApp.h"
#include "MainFrm.h"
#include "TimelineControl.h"
#include "BasicObjectsView.h"
#include "SlideView.h"
#include "WidgetControl.h"
#include "InspectorControlView.h"
#include "ActionView.h"
#include "IDragable.h"
#include "ActionView.h"
#include "ProjectView.h"
#include "TabOrderHandler.h"
#include "StudioPreferences.h"
#include "TimeLineToolbar.h"

#include <QtWidgets/qdockwidget.h>
#include <QtWidgets/qboxlayout.h>

//==============================================================================
/**
 * Constructor
 */
CPaletteManager::CPaletteManager(CMainFrame *inMainFrame)
    : m_MainFrame(inMainFrame)
{
    // Position tabs to the right
    inMainFrame->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::East);

    m_basicObjectsDock = new QDockWidget(QObject::tr("Basic Objects"), inMainFrame);
    m_basicObjectsDock->setObjectName("basic_objects");
    m_basicObjectsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                        | Qt::BottomDockWidgetArea);
    auto basicObjectsView = new BasicObjectsView(m_basicObjectsDock);
    m_basicObjectsDock->setWidget(basicObjectsView);
    inMainFrame->addDockWidget(Qt::RightDockWidgetArea, m_basicObjectsDock);
    m_ControlList.insert(std::make_pair(CONTROLTYPE_BASICOBJECTS, m_basicObjectsDock));

    m_projectDock = new QDockWidget(QObject::tr("Project"), inMainFrame);
    m_projectDock->setObjectName("project");
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                   | Qt::BottomDockWidgetArea);
    auto projectView = new ProjectView(m_projectDock);
    m_projectDock->setWidget(projectView);
    inMainFrame->addDockWidget(Qt::RightDockWidgetArea, m_projectDock);
    inMainFrame->tabifyDockWidget(m_basicObjectsDock, m_projectDock);
    m_ControlList.insert(std::make_pair(CONTROLTYPE_PROJECT, m_projectDock));

    m_slideDock = new QDockWidget(QObject::tr("Slide"), inMainFrame);
    m_slideDock->setObjectName("slide");
    m_slideDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    auto slideView = new SlideView(m_slideDock);
    m_slideDock->setWidget(slideView);
    inMainFrame->addDockWidget(Qt::LeftDockWidgetArea, m_slideDock);
    m_ControlList.insert(std::make_pair(CONTROLTYPE_SLIDE, m_slideDock));

    m_timelineDock = new QDockWidget(QObject::tr("Timeline"), inMainFrame);
    m_timelineDock->setObjectName("timeline");
    m_timelineDock->setAllowedAreas(Qt::BottomDockWidgetArea);

    QWidget *timeLineParent = new QWidget(inMainFrame);
    timeLineParent->setObjectName("TimeLineParent");
    m_timeLineToolbar = new TimeLineToolbar(inMainFrame, timeLineParent);
    QVBoxLayout *layout = new QVBoxLayout(timeLineParent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Use spacer widget instead of just layout spacing to get the color of the space correct
    QWidget *spacer = new QWidget(timeLineParent);
    spacer->setMaximumHeight(2);
    spacer->setMinimumHeight(2);

    auto c = new CTimelineControl();
    m_timeLineWidgetControl = new WidgetControl(c, timeLineParent);

    layout->addWidget(m_timeLineWidgetControl);
    layout->addWidget(spacer);
    layout->addWidget(m_timeLineToolbar);

    m_timelineDock->setWidget(timeLineParent);
    timeLineParent->setMinimumWidth(500);
    inMainFrame->addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);
    m_ControlList.insert(std::make_pair(CONTROLTYPE_TIMELINE, m_timelineDock));

    int actionViewMinWidth = CStudioPreferences::valueWidth()
            + CStudioPreferences::idWidth() + 40; // 40 added to accommodate tabs

    m_actionDock = new QDockWidget(QObject::tr("Action"), inMainFrame);
    m_actionDock->setObjectName("action");
    m_actionDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                  | Qt::BottomDockWidgetArea);
    auto actionView = new ActionView(m_actionDock);
    m_actionDock->setWidget(actionView);
    actionView->setMinimumWidth(actionViewMinWidth);
    inMainFrame->addDockWidget(Qt::BottomDockWidgetArea, m_actionDock);
    m_ControlList.insert(std::make_pair(CONTROLTYPE_ACTION, m_actionDock));

    m_inspectorDock = new QDockWidget(QObject::tr("Inspector Control"), inMainFrame);
    m_inspectorDock->setObjectName("inspector_control");
    m_inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                     | Qt::BottomDockWidgetArea);
    auto inspectorView = new InspectorControlView(m_inspectorDock);
    m_inspectorDock->setWidget(inspectorView);
    inspectorView->setMinimumWidth(actionViewMinWidth); // Same min size as action view
    inMainFrame->addDockWidget(Qt::BottomDockWidgetArea, m_inspectorDock);
    inMainFrame->tabifyDockWidget(m_actionDock, m_inspectorDock);
    m_ControlList.insert(std::make_pair(CONTROLTYPE_INSPECTOR, m_inspectorDock));

    m_basicObjectsDock->setEnabled(false);
    m_projectDock->setEnabled(false);
    m_slideDock->setEnabled(false);
    m_timelineDock->setEnabled(false);
    m_actionDock->setEnabled(false);
    m_inspectorDock->setEnabled(false);

    m_timeLineWidgetControl->RegiserForDnd(m_timeLineWidgetControl);
    m_timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_LISTBOX);
    m_timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_FILE);
    m_timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_ASSET_UICFILE);
    m_timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_ASSET_LIB);
    m_timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_ASSET_TL);
    m_timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_BASIC_OBJECTS);
}

//==============================================================================
/**
 * Destructor
 */
CPaletteManager::~CPaletteManager()
{
    TControlMap::iterator theIterator = m_ControlList.begin();
    TControlMap::iterator theEndIterator = m_ControlList.end();
    // Delete all the controls
    for (theIterator = m_ControlList.begin(); theIterator != theEndIterator; ++theIterator)
        delete theIterator->second;
}

//=============================================================================
/**
 *  Force a control to become invisible
 */
void CPaletteManager::HideControl(long inType)
{
    auto dock = GetControl(inType);

    if (dock) {
        // Make sure the control is invisible
        dock->setVisible(false);
    }
}
//=============================================================================
/**
 *	Detemine if a control is currently visible
 */
bool CPaletteManager::IsControlVisible(long inType) const
{
    auto dock = GetControl(inType);
    return dock && dock->isVisible();
}

//=============================================================================
/**
 *	Force a control to become visible
 */
void CPaletteManager::ShowControl(long inType)
{
    auto dock = GetControl(inType);

    if (dock) {
        // Make sure the control is visible
        dock->setVisible(true);
        dock->setFocus();
    }
}

//=============================================================================
/**
 *  Flip the visible state of a control
 */
void CPaletteManager::ToggleControl(long inType)
{
    if (IsControlVisible(inType))
        HideControl(inType);
    else
        ShowControl(inType);
}

//==============================================================================
/**
 *	Return the Control (Palette) according to its EControlTypes enum.
 *	@param inType	EControlTypes
 */
QDockWidget *CPaletteManager::GetControl(long inType) const
{
    auto dock = m_ControlList.find(inType);
    if (dock != m_ControlList.end() && dock->second)
        return dock->second;
    else
        return nullptr;
}

QWidget *CPaletteManager::getFocusWidget() const
{
    TControlMap::const_iterator end = m_ControlList.end();
    for (TControlMap::const_iterator iter = m_ControlList.begin(); iter != end; ++iter) {
        if (iter->second->widget()->hasFocus())
            return iter->second->widget();
    }
    return nullptr;
}

bool CPaletteManager::tabNavigateFocusedWidget(bool tabForward)
{
    QWidget *palette = getFocusWidget();
    if (palette) {
        if (auto inspector = qobject_cast<InspectorControlView *>(palette)) {
            inspector->tabOrderHandler()->tabNavigate(tabForward);
            return true;
        } else if (auto actionview = qobject_cast<ActionView *>(palette)) {
            actionview->tabOrderHandler()->tabNavigate(tabForward);
            return true;
        }
    }
    return false;
}

//==============================================================================
/**
 *  A helper for CMainFrame::GetTimelineControl() to access the CTimelineControl
 *  inside the QDockWidget
 */
CTimelineControl *CPaletteManager::GetTimelineControl() const
{
    if (m_timeLineWidgetControl)
        return static_cast<CTimelineControl *>(m_timeLineWidgetControl->getControl());

    return nullptr;
}

void CPaletteManager::onTimeChanged(long time)
{
    m_timeLineToolbar->onTimeChanged(time);
}

void CPaletteManager::EnablePalettes()
{
    m_basicObjectsDock->setEnabled(true);
    m_projectDock->setEnabled(true);
    m_slideDock->setEnabled(true);
    m_timelineDock->setEnabled(true);
    m_actionDock->setEnabled(true);
    m_inspectorDock->setEnabled(true);
}

