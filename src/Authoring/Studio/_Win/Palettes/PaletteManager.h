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
#include <bitset>
#include "MasterControl.h"
#include "TimelineControl.h"

//==============================================================================
//  Forwards
//==============================================================================
class CStudioApp;
class CMainFrame;
class CStudioPaletteBar;

QT_FORWARD_DECLARE_CLASS(QDockWidget)

//==============================================================================
/**
 *	@class	CPaletteManager
 */
class CPaletteManager : public IMasterControlProvider
{
public:
    // Do NOT change the order/values of this enum, these
    // values are stored in the registry
    enum EControlTypes {
        CONTROLTYPE_NONE = 0, ///<
        CONTROLTYPE_ACTION = 1, ///<
        CONTROLTYPE_BASICOBJECTS = 3, ///<
        CONTROLTYPE_INSPECTOR = 4, ///<
        CONTROLTYPE_SLIDE = 6, ///<
        CONTROLTYPE_TIMELINE = 7, ///<
        CONTROLTYPE_PROJECT = 9, ///<

        CONTROLTYPE_MAXCONTROLS = 32, ///< the maximum number of palettes( a string of this length
                                      ///is saved in the registry, changing this value will require
                                      ///an upgrade process )
    };

public:
    typedef std::bitset<CONTROLTYPE_MAXCONTROLS> TPaletteSet;

protected:
    typedef struct _TSMasterInfo
    {
        CStudioPaletteBar *m_PaletteBar; ///< pointer to the master palette bar
        TPaletteSet m_Palettes; ///< the palettes contained by the master
        CMasterControl *m_Master; ///< pointer to the master control

    } TSMasterInfo;

    typedef std::map<long, QDockWidget *> TControlMap;
    typedef std::vector<CStudioPaletteBar *> TPaletteList;
    typedef std::vector<TSMasterInfo> TMasterList;

protected:
    CMainFrame *m_MainFrame; ///<
    TControlMap m_ControlList; ///< map of EControlTypes and CControl
    TPaletteList m_PaletteList; ///<
    TMasterList m_MasterList; ///<

    static long s_PaletteIDBase; ///<

    QDockWidget *m_basicObjectsDock;
    QDockWidget *m_projectDock;
    QDockWidget *m_slideDock;
    QDockWidget *m_timelineDock;
    QDockWidget *m_actionDock;
    QDockWidget *m_inspectorDock;

public:
    CPaletteManager(CMainFrame *inMainFrame);
    virtual ~CPaletteManager();

    // Access
    void AddControlToMaster(CMasterControl *inControl, long inType);
    void RemoveControlFromMaster(CMasterControl *inControl, long inType);
    CMasterControl *FindMasterPalette(long inType);
    long GetMasterCount();
    CMasterControl *GetMaster(long inIndex);
    void HideControl(long inType);
    bool IsControlVisible(long inType) const;
    void ShowControl(long inType);
    void ToggleControl(long inType);

    QDockWidget *GetControl(long inType) const; ///< return corresponding Palette according to EControlTypes enum value
    QWidget *getFocusWidget() const;
    bool tabNavigateFocusedWidget(bool tabForward);
    CTimelineControl *GetTimelineControl() const;

    // Commands
    void OnNewPalette(CMasterControl *inMaster);
    void OnMovePalette(CMasterControl *inMoveFromMaster, CMasterControl *inMoveToMaster);
    void EnablePalettes();

    // Serialization
    bool Load();
    void Save();

    // Defaults
    void RestoreDefaults(bool inForce = false);

    // IMasterControlProvider
    void OnControlRemoved(CMasterControl *inControl) override;
    void OnContextMenu(CMasterControl *inControl, const CPt &inPosition,
                               CContextMenu *inMyMenu) override;
    void OnControlSelected(CMasterControl *inMaster, CControl *inNewControl, long inType) override;

    // Static Methods
    static Q3DStudio::CString GetControlName(long inType);

protected:
    CMasterControl *AddMasterPalette(long inType);

    void Reset();
    CStudioPaletteBar *CreatePalette();

    CMasterControl *FindUnusedMaster();
    CMasterControl *FindMasterByPaletteId(long inPaletteId);

    CStudioPaletteBar *FindStudioPaletteBar(long inType);
    CStudioPaletteBar *FindStudioPaletteBar(CMasterControl *inMaster);
    void OnAsyncDestroyWindow(CStudioPaletteBar *inWnd);
};

#endif // INCLUDED_VIEW_MANAGER_H
