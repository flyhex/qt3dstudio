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
#include "TimelineWidget.h"
#include "BasicObjectsView.h"
#include "SlideView.h"
#include "WidgetControl.h"
#include "InspectorControlView.h"
#include "ActionView.h"
#include "IDragable.h"
#include "ProjectView.h"
#include "TabOrderHandler.h"
#include "StudioPreferences.h"
#include "scenecameraview.h"

#include <QtWidgets/qdockwidget.h>
#include <QtWidgets/qboxlayout.h>

//==============================================================================
/**
 * Constructor
 */
CPaletteManager::CPaletteManager(CMainFrame *inMainFrame, QObject *parent)
    : QObject(parent)
    , m_MainFrame(inMainFrame)
{
    const int defaultBottomDockHeight = int(inMainFrame->height() * 0.25);
    const int defaultRightDockWidth = 435; // To fit all inspector controls
    const int defaultProjectHeight = 285; // To fit all new project folders, expanded

    // Position tabs to the right
    inMainFrame->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::East);
    inMainFrame->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    m_projectDock = new QDockWidget(QObject::tr("Project"), inMainFrame);
    m_projectDock->setObjectName(QStringLiteral("project"));
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                   | Qt::BottomDockWidgetArea);

    m_slideDock = new QDockWidget(QObject::tr("Slide"), inMainFrame);
    m_slideDock->setObjectName(QStringLiteral("slide"));
    m_slideDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    // Slide palette has a fixed size hint
    auto slideView = new SlideView(m_slideDock);
    slideView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_slideDock->setWidget(slideView);
    inMainFrame->addDockWidget(Qt::LeftDockWidgetArea, m_slideDock);
    m_ControlList.insert({CONTROLTYPE_SLIDE, m_slideDock});
    QObject::connect(m_slideDock, &QDockWidget::dockLocationChanged, slideView,
                     &SlideView::onDockLocationChange);

    m_basicObjectsDock = new QDockWidget(QObject::tr("Basic Objects"), inMainFrame);
    m_basicObjectsDock->setObjectName(QStringLiteral("basic_objects"));
    m_basicObjectsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                        | Qt::BottomDockWidgetArea);
    // Basic objects palette has a fixed size hint
    auto basicObjectsView = new BasicObjectsView(m_basicObjectsDock);
    basicObjectsView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_basicObjectsDock->setWidget(basicObjectsView);
    inMainFrame->addDockWidget(Qt::LeftDockWidgetArea, m_basicObjectsDock);
    inMainFrame->tabifyDockWidget(m_basicObjectsDock, m_slideDock);
    m_ControlList.insert({CONTROLTYPE_BASICOBJECTS, m_basicObjectsDock});

    m_timelineDock = new QDockWidget(QObject::tr("Timeline"));
    m_timelineDock->setObjectName(QStringLiteral("timeline"));
    m_timelineDock->setAllowedAreas(Qt::BottomDockWidgetArea);

    // Give the preferred size as percentages of the mainframe size
    m_timelineWidget = new TimelineWidget(QSize(inMainFrame->width() - defaultRightDockWidth,
                                                defaultBottomDockHeight));
    m_timelineWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    WidgetControl *timeLineWidgetControl = new WidgetControl(m_timelineWidget, m_timelineDock);
    timeLineWidgetControl->RegisterForDnd(timeLineWidgetControl);
    timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_FILE);
    timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_ASSET_UICFILE);
    timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_ASSET_LIB);
    timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_ASSET_TL);
    timeLineWidgetControl->AddMainFlavor(QT3DS_FLAVOR_BASIC_OBJECTS);

    m_timelineWidget->setParent(timeLineWidgetControl);

    m_timelineDock->setWidget(timeLineWidgetControl);
    inMainFrame->addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);
    m_ControlList.insert({CONTROLTYPE_TIMELINE, m_timelineDock});

    m_cameraDock = new QDockWidget(QObject::tr("Scene Camera"));
    m_cameraDock->setObjectName(QStringLiteral("scenecamera"));
    m_cameraDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea
                                  | Qt::RightDockWidgetArea);

    m_cameraWidget = new SceneCameraView(inMainFrame, m_cameraDock);
    m_cameraWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_cameraDock->setWidget(m_cameraWidget);
    inMainFrame->addDockWidget(Qt::BottomDockWidgetArea, m_cameraDock);
    inMainFrame->tabifyDockWidget(m_timelineDock, m_cameraDock);
    m_ControlList.insert({CONTROLTYPE_SCENECAMERA, m_cameraDock});

    // Give the preferred size as percentages of the mainframe size
    m_projectView = new ProjectView(QSize(defaultRightDockWidth, defaultProjectHeight),
                                    m_projectDock);
    m_projectView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_projectDock->setWidget(m_projectView);
    inMainFrame->addDockWidget(Qt::RightDockWidgetArea, m_projectDock);
    m_ControlList.insert({CONTROLTYPE_PROJECT, m_projectDock});

    m_actionDock = new QDockWidget(QObject::tr("Action"), inMainFrame);
    m_actionDock->setObjectName(QStringLiteral("action"));
    m_actionDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                  | Qt::BottomDockWidgetArea);
    // Give the preferred size as percentages of the mainframe size
    auto actionView = new ActionView(
                QSize(defaultRightDockWidth, inMainFrame->height() - defaultProjectHeight),
                m_actionDock);
    actionView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_actionDock->setWidget(actionView);
    inMainFrame->addDockWidget(Qt::RightDockWidgetArea, m_actionDock);
    m_ControlList.insert({CONTROLTYPE_ACTION, m_actionDock});

    m_inspectorDock = new QDockWidget(QObject::tr("Inspector"), inMainFrame);
    m_inspectorDock->setObjectName(QStringLiteral("inspector_control"));
    m_inspectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea
                                     | Qt::BottomDockWidgetArea);
    // Give the preferred size as percentages of the mainframe size
    auto inspectorView = new InspectorControlView(
                QSize(defaultRightDockWidth, inMainFrame->height() - defaultProjectHeight),
                m_inspectorDock);
    inspectorView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_inspectorDock->setWidget(inspectorView);
    inMainFrame->addDockWidget(Qt::RightDockWidgetArea, m_inspectorDock);
    inMainFrame->tabifyDockWidget(m_inspectorDock, m_actionDock);
    m_ControlList.insert({CONTROLTYPE_INSPECTOR, m_inspectorDock});

    m_inspectorDock->raise();

    EnablePalettes(false);
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

ProjectView *CPaletteManager::projectView() const
{
    return m_projectView;
}

void CPaletteManager::EnablePalettes(bool enable)
{
    m_basicObjectsDock->setEnabled(enable);
    m_projectDock->setEnabled(enable);
    m_slideDock->setEnabled(enable);
    m_timelineDock->setEnabled(enable);
    m_actionDock->setEnabled(enable);
    m_inspectorDock->setEnabled(enable);
    m_cameraDock->setEnabled(enable);
}

