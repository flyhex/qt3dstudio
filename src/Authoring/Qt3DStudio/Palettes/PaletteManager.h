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
//	Prefix
//==============================================================================
#ifndef INCLUDED_VIEW_MANAGER_H
#define INCLUDED_VIEW_MANAGER_H 1

//==============================================================================
//	Includes
//==============================================================================
#include <QtWidgets/qdockwidget.h>
#include <QtCore/qobject.h>

//==============================================================================
//  Forwards
//==============================================================================
class CMainFrame;
class WidgetControl;
class TimeLineToolbar;
class TimelineView;
class ProjectView;
class TimelineWidget;
class SceneCameraView;

QT_FORWARD_DECLARE_CLASS(QDockWidget)

//==============================================================================
/**
 *	@class	CPaletteManager
 */
class CPaletteManager : public QObject
{
    Q_OBJECT
public:
    enum EControlTypes {
        CONTROLTYPE_NONE = 0,
        CONTROLTYPE_ACTION,
        CONTROLTYPE_BASICOBJECTS,
        CONTROLTYPE_INSPECTOR,
        CONTROLTYPE_SLIDE,
        CONTROLTYPE_TIMELINE,
        CONTROLTYPE_PROJECT,
        CONTROLTYPE_SCENECAMERA,
    };

protected:
    typedef std::map<long, QDockWidget *> TControlMap;

protected:
    CMainFrame *m_MainFrame;
    TControlMap m_ControlList;

    QDockWidget *m_basicObjectsDock;
    QDockWidget *m_projectDock;
    QDockWidget *m_slideDock;
    QDockWidget *m_timelineQmlDock;
    QDockWidget *m_timelineDock;
    QDockWidget *m_actionDock;
    QDockWidget *m_inspectorDock;
    QDockWidget *m_cameraDock;

    TimelineView *m_timelineView;
    ProjectView *m_projectView = nullptr;
    TimelineWidget *m_timelineWidget;
    SceneCameraView *m_cameraWidget;

public:
    CPaletteManager(CMainFrame *inMainFrame, QObject *parent = nullptr);
    virtual ~CPaletteManager();

    // Access
    void HideControl(long inType);
    bool IsControlVisible(long inType) const;
    void ShowControl(long inType);
    void ToggleControl(long inType);
    QDockWidget *GetControl(long inType) const;
    QWidget *getFocusWidget() const;
    bool tabNavigateFocusedWidget(bool tabForward);
    ProjectView *projectView() const;

    // Commands
    void EnablePalettes(bool enable = true);
};

#endif // INCLUDED_VIEW_MANAGER_H
