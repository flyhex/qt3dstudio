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
#include "stdafx.h"
#include "Strings.h"

//==============================================================================
//	Includes
//==============================================================================
#include "PaletteManager.h"
#include "StudioApp.h"
#include "Views.h"
#include "MainFrm.h"
#include "StringLoader.h"
#include "TimelineControl.h"
#include "BasicObjectsView.h"
#include "SlideView.h"
#include "StringTokenizer.h"
#include "Preferences.h"
#include "WidgetControl.h"
#include "InspectorControlView.h"
#include "ActionView.h"
#include "IDragable.h"
#include "ActionView.h"
#include "ProjectView.h"
#include "TabOrderHandler.h"

#include <QDockWidget>

//==============================================================================
/**
 *	Class for std::for_each to delete each control
 */
template <class T>
class CDeleteAll
{
public:
    void operator()(const T *inControl) const { delete inControl; }
};

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
    auto c = new CTimelineControl();
    auto w = new WidgetControl(c, m_timelineDock);
    m_timelineDock->setWidget(w);
    w->setMinimumWidth(500);
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

    w->RegiserForDnd(w);
    w->AddMainFlavor(QT3DS_FLAVOR_LISTBOX);
    w->AddMainFlavor(QT3DS_FLAVOR_FILE);
    w->AddMainFlavor(QT3DS_FLAVOR_ASSET_UICFILE);
    w->AddMainFlavor(QT3DS_FLAVOR_ASSET_LIB);
    w->AddMainFlavor(QT3DS_FLAVOR_ASSET_TL);
    w->AddMainFlavor(QT3DS_FLAVOR_BASIC_OBJECTS);

    // Set to a default state
    Reset();
}

//==============================================================================
/**
 * Destructor
 */
CPaletteManager::~CPaletteManager()
{
    TControlMap::iterator theIterator = m_ControlList.begin();
    TControlMap::iterator theEndIterator = m_ControlList.end();
#ifdef KDAB_TEMPORARILY_REMOVED
    for (; theIterator != theEndIterator; ++theIterator) {
        // Remove the palette from the old master
        long theType = theIterator->first;
        CMasterControl *theOldMaster = FindMasterPalette(theType);
        if (theOldMaster)
            RemoveControlFromMaster(theOldMaster, theType);
    }
    std::for_each(m_PaletteList.begin(), m_PaletteList.end(), CDeleteAll<CStudioPaletteBar>());
#endif
    // Delete all the controls
    for (theIterator = m_ControlList.begin(); theIterator != theEndIterator; ++theIterator)
        delete theIterator->second;
}

//==============================================================================
/**
 *	Clear out old lists and reset all the palettes
 */
void CPaletteManager::Reset()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    // Set the visible state to false
    TControlMap::iterator theIterator = m_ControlList.begin();
    TControlMap::iterator theEndIterator = m_ControlList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        CControl *theControl = theIterator->second;
        if (theControl)
            theControl->SetVisible(false);
    }

    // Undock all palette
    TPaletteList::iterator thePaletteIterator = m_PaletteList.begin();
    for (; thePaletteIterator != m_PaletteList.end(); ++thePaletteIterator) {
        CStudioPaletteBar *thePaletteBar = (*thePaletteIterator);
        thePaletteBar->ShowPalette(false);
    }
#endif
}

//==============================================================================
/**
 *	Create a palette
 */
CStudioPaletteBar *CPaletteManager::CreatePalette()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    // Build the create context
    CCreateContext theCreateContext;
    memset(&theCreateContext, 0, sizeof(CCreateContext));
    theCreateContext.m_pNewViewClass = RUNTIME_CLASS(CMasterView);
    theCreateContext.m_pCurrentFrame = m_MainFrame;

    // Create the palette bar
    CStudioPaletteBar *thePaletteBar = new CStudioPaletteBar;
    thePaletteBar->Create(::CString("<empty>"), &theCreateContext, GetUniquePaletteId(),
                          m_MainFrame);

    // Set the provider
    CMasterView *theMasterView = (CMasterView *)thePaletteBar->GetView();
    theMasterView->SetProvider(this);

    // Must occur after the provider is set
    thePaletteBar->InitialUpdate();

    // Save the palette
    ASSERT(thePaletteBar != nullptr);
    m_PaletteList.push_back(thePaletteBar);

    // Store the information
    TSMasterInfo theInfo;
    theInfo.m_Master = theMasterView->GetMasterControl();
    theInfo.m_PaletteBar = thePaletteBar;

    ASSERT(theInfo.m_PaletteBar != nullptr);
    m_MasterList.push_back(theInfo);

    return thePaletteBar;
#endif
    return nullptr;
}

//==============================================================================
/**
 *	Find a master control with no contained controls
 */
CMasterControl *CPaletteManager::FindUnusedMaster()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    TMasterList::iterator theIterator = m_MasterList.begin();
    TMasterList::iterator theEndIterator = m_MasterList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        if (theIterator->m_Palettes.any() == false)
            return theIterator->m_Master;
    }
#endif

    return nullptr;
}

CMasterControl *CPaletteManager::FindMasterByPaletteId(long inPaletteId)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    TMasterList::iterator theIterator = m_MasterList.begin();
    TMasterList::iterator theEndIterator = m_MasterList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        CControlBarInfo theBarInfo;
        theIterator->m_PaletteBar->GetBarInfo(&theBarInfo);

        if (theBarInfo.m_nBarID == (UINT)inPaletteId)
            return theIterator->m_Master;
    }
#endif
    return nullptr;
}

CStudioPaletteBar *CPaletteManager::FindStudioPaletteBar(long inType)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    TMasterList::iterator theIterator = m_MasterList.begin();
    TMasterList::iterator theEndIterator = m_MasterList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        if (theIterator->m_Palettes[inType] == true)
            return theIterator->m_PaletteBar;
    }
#endif
    return nullptr;
}

CStudioPaletteBar *CPaletteManager::FindStudioPaletteBar(CMasterControl *inMaster)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    TMasterList::iterator theIterator = m_MasterList.begin();
    TMasterList::iterator theEndIterator = m_MasterList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        if (theIterator->m_Master == inMaster)
            return theIterator->m_PaletteBar;
    }
#endif
    return nullptr;
}

//==============================================================================
/**
 * Add a new master to the list of masters
 */
CMasterControl *CPaletteManager::AddMasterPalette(long inType)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    // Remove the palette from the old master
    CMasterControl *theOldMaster = FindMasterPalette(inType);
    if (theOldMaster)
        RemoveControlFromMaster(theOldMaster, inType);

    // Create a new master
    CMasterControl *theMasterControl = FindUnusedMaster();
    if (theMasterControl != nullptr) {
        // Add the palette to the new master
        AddControlToMaster(theMasterControl, inType);
    }

    return theMasterControl;
#endif
    return nullptr;
}

//==============================================================================
/**
 *	Add the specified palette to the specified master
 */
void CPaletteManager::AddControlToMaster(CMasterControl *inMaster, long inType)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    // Make sure this is a valid type ( for backward compatibility )
    TControlMap::iterator theControlIterator = m_ControlList.find(inType);
    if (theControlIterator != m_ControlList.end()) {
        // Make sure the palette is not already on another master
        CMasterControl *theOldMaster = FindMasterPalette(inType);
        if (theOldMaster)
            RemoveControlFromMaster(theOldMaster, inType);

        // Add the palette to this master
        TMasterList::iterator theIterator = m_MasterList.begin();
        for (; theIterator != m_MasterList.end(); ++theIterator) {
            if (theIterator->m_Master == inMaster) {
                theIterator->m_Palettes[inType] = true;
                theIterator->m_Master->AddControl(GetControlName(inType), inType,
                                                  theControlIterator->second);
                theIterator->m_Master->Invalidate();
                break;
            }
        }
    }
#endif
}

//==============================================================================
/**
 *	Remove the specified palette from the specified master
 */
void CPaletteManager::RemoveControlFromMaster(CMasterControl *inMaster, long inType)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    TMasterList::iterator theIterator = m_MasterList.begin();
    for (; theIterator != m_MasterList.end(); ++theIterator) {
        if (theIterator->m_Master == inMaster) {
            theIterator->m_Palettes[inType] = false;
            theIterator->m_Master->RemoveControl(GetControl(inType));
            theIterator->m_Master->Invalidate();
        }
    }
#endif
}

//==============================================================================
/**
 *	Find the name of the master that contains this palette
 */
CMasterControl *CPaletteManager::FindMasterPalette(long inType)
{
    CMasterControl *theMaster = nullptr;

#ifdef KDAB_TEMPORARILY_REMOVED
    TMasterList::iterator theIterator = m_MasterList.begin();
    TMasterList::iterator theEndIterator = m_MasterList.end();
    for (; theIterator != theEndIterator; ++theIterator) {
        if (theIterator->m_Palettes[inType] == true) {
            theMaster = theIterator->m_Master;
            break;
        }
    }
#endif
    return theMaster;
}

//==============================================================================
/**
 *	Query the number of masters
 */
long CPaletteManager::GetMasterCount()
{
    return (long)m_MasterList.size();
}

//==============================================================================
/**
 *	Get the master at the specified index
 */
CMasterControl *CPaletteManager::GetMaster(long inIndex)
{
    CMasterControl *theControl = nullptr;
#ifdef KDAB_TEMPORARILY_REMOVED
    TMasterList::iterator theIterator = m_MasterList.begin();
    std::advance(theIterator, inIndex);
    if (theIterator != m_MasterList.end())
        theControl = theIterator->m_Master;
#endif
    return theControl;
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
    auto dock = GetControl(CPaletteManager::CONTROLTYPE_TIMELINE);

    if (dock) {
        auto widget = static_cast<WidgetControl *>(dock->widget());

        if (widget)
            return static_cast<CTimelineControl *>(widget->getControl());
    }

    return nullptr;
}

//==============================================================================
//	Serialization
//==============================================================================

//==============================================================================
/**
 *	Load the palette state from the registry
 */
bool CPaletteManager::Load()
{
    // Clear out existing local stuff
    Reset();

#ifdef KDAB_TEMPORARILY_REMOVED
    // Test the palette prefs, return false if they don't exist
    if (CPreferences::GetUserPreferences().Exists(PALETTE_KEY) == false)
        return false;

    long theCurrentPalette = 0;
    while (true) {
        // Format the subkey name
        Q3DStudio::CString theSubKeyName;
        theSubKeyName.Format(L"%ls%ld", PALETTE_SUBKEY, theCurrentPalette);

        // Check to see if we should exit the for loop
        CPreferences thePalettePrefs = CPreferences::GetUserPreferences(PALETTE_KEY);
        if (thePalettePrefs.Exists(theSubKeyName) == false)
            break;

        // Format the subkey name
        Q3DStudio::CString theFullKeyName;
        theFullKeyName.Format(L"%ls\\%ls%ld", PALETTE_KEY, PALETTE_SUBKEY, theCurrentPalette);

        // Grab the prefs
        CPreferences thePrefs = CPreferences::GetUserPreferences(theFullKeyName);

        // Default docking value
        Q3DStudio::CString theDockString =
            thePrefs.GetStringValue(PALETTE_CONTROL_DOCKID, L"0"); // left
        long theDockId = ::atol(theDockString.GetCharStar());

        // Default Rect
        QRect theWindowRect;
        Q3DStudio::CString theRectString =
            thePrefs.GetStringValue(PALETTE_CONTROL_RECT, L"0,0,0,0");
        CStringTokenizer theRectTokenizer(theRectString, L",");
        for (long theIndex = 0; theRectTokenizer.HasNextPartition();
             ++theRectTokenizer, ++theIndex) {
            Q3DStudio::CString theCurrentString = theRectTokenizer.GetCurrentPartition();
            long theCurrentItem = ::atol(theCurrentString.GetCharStar());

            switch (theIndex) {
            case 0:
                theWindowRect.setLeft(theCurrentItem);
                break;
            case 1:
                theWindowRect.setTop(theCurrentItem);
                break;
            case 2:
                theWindowRect.setRight(theCurrentItem);
                break;
            case 3:
                theWindowRect.setBottom(theCurrentItem);
                break;
            };
        }

        // Palette id
        Q3DStudio::CString thePaletteIdString =
            thePrefs.GetStringValue(PALETTE_CONTROL_PALETTEID, L"0");
        long thePaletteId = ::atol(thePaletteIdString.GetCharStar());

        // Read the selected item
        Q3DStudio::CString theSelectedString =
            thePrefs.GetStringValue(PALETTE_CONTROL_SELECTED, L"0");
        long theSelectedType = ::atol(theSelectedString.GetCharStar());

        // Grab the palette
        CMasterControl *theMasterControl;
        if (thePaletteId)
            theMasterControl = FindMasterByPaletteId(thePaletteId);
        else
            theMasterControl = FindUnusedMaster();

        if (theMasterControl != nullptr) {
            CStudioPaletteBar *thePaletteBar = FindStudioPaletteBar(theMasterControl);

            Q3DStudio::CString theContentString =
                thePrefs.GetStringValue(PALETTE_CONTROL_LIST, L"0");
            CStringTokenizer theTokenizer(theContentString, L",");
            for (; theTokenizer.HasNextPartition(); ++theTokenizer) {
                Q3DStudio::CString theCurrentString = theTokenizer.GetCurrentPartition();
                long theCurrentItem = ::atol(theCurrentString.GetCharStar());

                if (theCurrentItem != CONTROLTYPE_NONE)
                    AddControlToMaster(theMasterControl, theCurrentItem);
            }

            // Dock the bar in the appropriate location (if requested)
            if (theDockId != 0) {
                CDockBar *theDockBar =
                    (CDockBar *)m_MainFrame->GetControlBar(theDockId + AFX_IDW_DOCKBAR_TOP);
                theDockBar->ClientToScreen(theWindowRect);

                thePaletteBar->SetHorz(CSize(theWindowRect.Width(), theWindowRect.Height()));
                thePaletteBar->SetVert(CSize(theWindowRect.Width(), theWindowRect.Height()));

                m_MainFrame->DockControlBar(thePaletteBar, theDockBar, theWindowRect);
                m_MainFrame->RecalcLayout(TRUE);
            }

            // Select the old item
            theMasterControl->SelectControl(theSelectedType);

            // Turn on my heart light
            thePaletteBar->ShowPalette(true);
        }

        // Move on to the next key
        theCurrentPalette++;
    }

    if (m_MainFrame) {
        // Allow the control bars (docking palettes, etc) the opportunity to restore
        // This will use a registry key based off of AFX_IDS_APP_TITLE ("Qt 3D Studio") which
        // is different than CPreferences (".../Qt 3D Studio/Settings/...")
        CSizingControlBar::GlobalLoadState(m_MainFrame, PALETTE_LAYOUT_KEY_NAME);
        m_MainFrame->LoadBarState(PALETTE_LAYOUT_KEY_NAME);
    }
#endif

    return true;
}

//==============================================================================
/**
 *	Save the palette state to the registry
 */
void CPaletteManager::Save()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    // Destroy current information in the prefs
    CPreferences theMasterPrefs = CPreferences::GetUserPreferences(PALETTE_KEY);
    theMasterPrefs.Clear();

    // Iterate through the m_MasterList
    TMasterList::iterator theIterator = m_MasterList.begin();
    TMasterList::iterator theEndIterator = m_MasterList.end();
    for (long theMasterIndex = 0; theIterator != theEndIterator; ++theIterator) {
        TSMasterInfo &theMasterInfo = *theIterator;
        if (theMasterInfo.m_PaletteBar->IsVisible()) {
            Q3DStudio::CString theSubKeyName;
            theSubKeyName.Format(L"%ls\\%ls%ld", PALETTE_KEY, PALETTE_SUBKEY, theMasterIndex);

            CPreferences thePrefs = CPreferences::GetUserPreferences(theSubKeyName);

            // Save palette contents
            Q3DStudio::CString theTempString;
            Q3DStudio::CString theContentString;
            for (long theIndex = 0; theIndex < (long)theMasterInfo.m_Palettes.size(); ++theIndex) {
                if (theMasterInfo.m_Palettes[theIndex]) {
                    theTempString.Format(L"%ld,", theIndex);
                    theContentString += theTempString;
                }
            }

            thePrefs.SetStringValue(PALETTE_CONTROL_LIST, theContentString);

            // Save palette id
            CControlBarInfo theBarInfo;
            theMasterInfo.m_PaletteBar->GetBarInfo(&theBarInfo);
            theTempString.Format(L"%ld", theBarInfo.m_nBarID);
            thePrefs.SetStringValue(PALETTE_CONTROL_PALETTEID, theTempString);

            // Save selected item
            long theSelectedIndex = theMasterInfo.m_Master->GetActiveIndex();
            theTempString.Format(L"%ld", theSelectedIndex);
            thePrefs.SetStringValue(PALETTE_CONTROL_SELECTED, theTempString);

            // Default values
            thePrefs.SetStringValue(PALETTE_CONTROL_DOCKID, L"0");
            thePrefs.SetStringValue(PALETTE_CONTROL_RECT, L"0,0,0,0");

            // Increment the palette counter
            ++theMasterIndex;
        }
    }

    // Allow the control bars (docking palettes, etc) the opportunity to store
    // This will use a registry key based off of AFX_IDS_APP_TITLE ("Qt 3D Studio") which
    // is different than CPreferences (".../Qt 3D Studio/Settings/...")
    if (m_MainFrame) {
        CSizingControlBar::GlobalSaveState(m_MainFrame, PALETTE_LAYOUT_KEY_NAME);
        m_MainFrame->SaveBarState(PALETTE_LAYOUT_KEY_NAME);
    }
#endif
}

//==============================================================================
//	Defaults
//==============================================================================

//==============================================================================
/**
 *	Set the palettes to the default state
 *
 *	CONTROLTYPE_ACTION			= 1,	///<
 *	CONTROLTYPE_BASICOBJECTS	= 3,	///<
 *	CONTROLTYPE_INSPECTOR		= 4,	///<
 *	CONTROLTYPE_SLIDE			= 6,	///<
 *	CONTROLTYPE_TIMELINE		= 7,	///<
 *	CONTROLTYPE_COMMAND			= 8,	///<
 *	CONTROLTYPE_PROJECT			= 9,	///<
 *
 *	#define AFX_IDW_DOCKBAR_TOP             0xE81B  = 0
 *	#define AFX_IDW_DOCKBAR_LEFT            0xE81C  = 1
 *	#define AFX_IDW_DOCKBAR_RIGHT           0xE81D  = 2
 *	#define AFX_IDW_DOCKBAR_BOTTOM          0xE81E  = 3
 *	#define AFX_IDW_DOCKBAR_FLOAT           0xE81F  = 4
 */
void CPaletteManager::RestoreDefaults(bool inForce)
{
    Q_UNUSED(inForce);
#ifdef KDAB_TEMPORARILY_REMOVED

    // Destroy current information in the prefs (Palettes)
    CPreferences theMasterPrefs = CPreferences::GetUserPreferences(PALETTE_KEY);
    theMasterPrefs.Clear();

    // "3,9,2,5:0:2:0,0,300,250#4,1:0:2:0,251,300,601#6:0:1:0,0,125,125#7:0:3:0,0,250,300";
    Q3DStudio::CString thePaletteString = ::LoadResourceString(IDS_DEFAULT_PALETTE_LAYOUT);
    CStringTokenizer thePaletteTokenizer(thePaletteString, L"#");
    for (long thePaletteIndex = 0; thePaletteTokenizer.HasNextPartition();
         ++thePaletteTokenizer, ++thePaletteIndex) {
        Q3DStudio::CString theCurrentPalette = thePaletteTokenizer.GetCurrentPartition();
        CStringTokenizer theSubTokenizer(theCurrentPalette, L":");

        // No error checking, this string better be right!
        Q3DStudio::CString theControlList = theSubTokenizer.GetCurrentPartition();
        ++theSubTokenizer;
        Q3DStudio::CString theSelectedControl = theSubTokenizer.GetCurrentPartition();
        ++theSubTokenizer;
        Q3DStudio::CString theDockId = theSubTokenizer.GetCurrentPartition();
        ++theSubTokenizer;
        Q3DStudio::CString theControlRect = theSubTokenizer.GetCurrentPartition();
        ++theSubTokenizer;

        // Create the new registry keys (Palettes\PaletteN)
        Q3DStudio::CString theSubKeyName;
        theSubKeyName.Format(L"%ls\\%s%ld", PALETTE_KEY, PALETTE_SUBKEY, thePaletteIndex);
        CPreferences thePrefs = CPreferences::GetUserPreferences(theSubKeyName);

        thePrefs.SetStringValue(PALETTE_CONTROL_LIST, theControlList);
        thePrefs.SetStringValue(PALETTE_CONTROL_SELECTED, theSelectedControl);
        thePrefs.SetStringValue(PALETTE_CONTROL_DOCKID, theDockId);
        thePrefs.SetStringValue(PALETTE_CONTROL_RECT, theControlRect);
    }
#endif
    // Load these items
    Load();
}

//==============================================================================
//	Palette Context Menu
//==============================================================================

//=============================================================================
/**
 *	Callback to create a new palette of the specified type.
 */
void CPaletteManager::OnNewPalette(CMasterControl *inMaster)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    ASSERT(inMaster != nullptr);

    // Query the current control type
    long theType = inMaster->GetActiveType();

    // Add a new master with that control type
    CMasterControl *theNewMaster = AddMasterPalette(theType);

    // Float the new palette
    CStudioPaletteBar *thePaletteBar = FindStudioPaletteBar(theNewMaster);
    m_MainFrame->FloatControlBar(thePaletteBar, QPoint(0, 0));

    // Turn on my heart light
    thePaletteBar->ShowPalette(true);
#endif
}

//=============================================================================
/**
 *	Callback to move a control to the specified palette
 */
void CPaletteManager::OnMovePalette(CMasterControl *inMoveFromMaster,
                                    CMasterControl *inMoveToMaster)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    ASSERT(inMoveFromMaster != nullptr);
    ASSERT(inMoveToMaster != nullptr);

    // Move the control to the new master
    long theType = inMoveFromMaster->GetActiveType();
    AddControlToMaster(inMoveToMaster, theType);

    // Check to see if the master is empty
    if (inMoveFromMaster->GetControlCount() <= 0) {
        // Make sure the palette is visible
        CStudioPaletteBar *thePaletteBar = FindStudioPaletteBar(inMoveFromMaster);
        thePaletteBar->ShowPalette(false);
    }
#endif
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

//==============================================================================
//	IMasterControlProvider
//==============================================================================

void CPaletteManager::OnControlRemoved(CMasterControl *inMaster)
{
    (void)inMaster;
}

void CPaletteManager::OnContextMenu(CMasterControl *inMaster, const CPt &inPosition,
                                    CContextMenu *inMyMenu)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    CPaletteContextMenu theContextMenu(this, inMaster, inMyMenu);

    // Popup the context menu
    inMaster->DoPopup(&theContextMenu, inPosition);
#endif
}

void CPaletteManager::OnControlSelected(CMasterControl *inMaster, CControl *, long inType)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    Q3DStudio::CString theControlName = GetControlName(inType);

    CStudioPaletteBar *thePalette = FindStudioPaletteBar(inMaster);
    ASSERT(thePalette != nullptr);

    thePalette->SetWindowText(theControlName);
#endif
}

//==============================================================================
//	Static Methods
//==============================================================================

//==============================================================================
/**
 *	Query the name of the specified palette
 */
Q3DStudio::CString CPaletteManager::GetControlName(long inType)
{
    switch (inType) {
    case CONTROLTYPE_ACTION:
        return ::LoadResourceString(IDS_PALETTE_ACTION);
    case CONTROLTYPE_BASICOBJECTS:
        return ::LoadResourceString(IDS_PALETTE_BASIC_OBJECTS);
    case CONTROLTYPE_INSPECTOR:
        return ::LoadResourceString(IDS_PALETTE_INSPECTOR);
    case CONTROLTYPE_SLIDE:
        return ::LoadResourceString(IDS_PALETTE_SLIDE);
    case CONTROLTYPE_TIMELINE:
        return ::LoadResourceString(IDS_PALETTE_TIMELINE);
    case CONTROLTYPE_PROJECT:
        return ::LoadResourceString(IDS_PALETTE_PROJECT);
    default:
        return L"< Empty >";
    };
}

