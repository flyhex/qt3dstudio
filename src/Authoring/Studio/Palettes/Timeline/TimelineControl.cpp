/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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

//==============================================================================
//	Includes
//==============================================================================
#include "TimelineControl.h"
#include "TimelineSplitter.h"
#include "StudioApp.h"
#include "Dispatch.h"
#include "TimelineTreeLayout.h"
#include "TimelineTimelineLayout.h"
#include "SlideRow.h"
#include "IDoc.h"
#include "InsertionLine.h"
#include "InsertionOverlay.h"
#include "Renderer.h"
#include "StudioPreferences.h"
#include "BreadCrumbControl.h"
#include "BaseTimelineTreeControl.h"
#include "Bindings/TimelineTranslationManager.h"
#include "Doc.h"
#include "Core.h"
#include "MasterP.h"

// Data model specific
#include "TimelineDropTarget.h"

#include "ClientDataModelBridge.h"
#include "UICDMStudioSystem.h"
#include "UICDMSlides.h"

IMPLEMENT_OBJECT_COUNTER(CTimelineControl)

CTimelineControl::CTimelineControl()
    : m_SuspendRecalcLayout(false)
    , m_TranslationManager(nullptr)
{
    ADDTO_OBJECT_COUNTER(CTimelineControl)

    m_TranslationManager = new CTimelineTranslationManager();

    m_Splitter = new CTimelineSplitter();
    AddChild(m_Splitter);

    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    m_TreeLayout = new CTimelineTreeLayout(this, theDoc);
    m_Splitter->AddChild(m_TreeLayout);

    m_TimelineLayout = new CTimelineTimelineLayout(this, theDoc);
    m_Splitter->AddChild(m_TimelineLayout);

    m_Splitter->SetSplitDirection(CSplitter::SPLIT_VERTICAL);
    m_Splitter->SetSplitLocation(CStudioPreferences::GetTimelineSplitterLocation());

    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    theDispatch->AddPresentationChangeListener(this);
    theDispatch->AddClientPlayChangeListener(this);

    // Insertion line
    m_InsertionLine = new CInsertionLine();
    m_InsertionLine->SetName("TimelineInsertionLine");
    AddChild(m_InsertionLine);

    // Insertion overlay marker
    m_InsertionOverlay = new CInsertionOverlay();
    m_InsertionOverlay->SetName("TimelineInsertionOverlay");
    AddChild(m_InsertionOverlay);

    m_Splitter->SetPosition(CPt(0, CStudioPreferences::GetHeaderHeight()));

    m_BreadCrumbToolbar = new CBreadCrumbControl();
    AddChild(m_BreadCrumbToolbar);

    SetPreferredSize(CPt(400, 200));
}

CTimelineControl::~CTimelineControl()
{
    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    theDispatch->RemovePresentationChangeListener(this);
    theDispatch->RemoveClientPlayChangeListener(this);

    delete m_InsertionOverlay;
    delete m_InsertionLine;
    delete m_TimelineLayout;
    delete m_TreeLayout;
    delete m_Splitter;
    delete m_BreadCrumbToolbar;

    REMOVEFROM_OBJECT_COUNTER(CTimelineControl)
}

//=============================================================================
/**
  * Returns the playhead time
  */
long CTimelineControl::GetTime()
{
    return m_TreeLayout->GetTime();
}

//=============================================================================
/**
 * Clear the contents of this view.
 * This will empty out this view and leave it ready for inspecting other objects.
 */
void CTimelineControl::ClearView()
{
    m_TimelineLayout->ClearRows();
    m_TreeLayout->ClearRows();
    m_ActiveSlide = 0;

    // clean out all previous translations, because the bindings are not guaranteed to be valid when
    // switching from one slide to another.
    m_TranslationManager->Clear();
}

//=============================================================================
/**
 * Populates this view with the provided state.
 * This will set the state as being the root object on this view. ClearView
 * should be called before this is called.
 * The object will become the root object of this and will become the active
 * root of the doc.
 * @param inState the state to be viewed as the root asset.
 */
void CTimelineControl::ViewSlide(qt3dsdm::CUICDMSlideHandle inSlide)
{
    m_ActiveSlide = inSlide;

    qt3dsdm::ISlideSystem *theSlideSystem = GetDoc()->GetStudioSystem()->GetSlideSystem();
    qt3dsdm::CUICDMInstanceHandle theSlideInstance = theSlideSystem->GetSlideInstance(inSlide);
    CSlideRow *theSlideRow = new CSlideRow(m_TranslationManager->GetOrCreate(theSlideInstance));
    theSlideRow->SetTimelineControl(this);

    m_TreeLayout->AddRow(theSlideRow);
    m_TimelineLayout->AddRow(theSlideRow);

    // Since this would be loading the entire context's assets, fire the OnTimelineLayoutChange
    // event just once.
    SuspendLayoutChanges(true);
    try {
        theSlideRow->LoadChildren();
        theSlideRow->Expand();
    } catch (...) { // restore the 'states' before passing the exception up
        SuspendLayoutChanges(false);
        throw;
    }
    // Update breadcrumbs
    m_BreadCrumbToolbar->RefreshTrail(m_TranslationManager->GetBreadCrumbProvider());

    SuspendLayoutChanges(false);
    OnLayoutChanged();
}

//=============================================================================
/**
 * Notification from the StudioFullSystem signal provider that a we have a new active slide.
 * This will populate this view with the new context.
 */
void CTimelineControl::OnActiveSlide(qt3dsdm::CUICDMSlideHandle inSlide)
{
    ClearView();
    ViewSlide(inSlide);

    double theStoredRatio = m_TimelineLayout->GetTimelineRatio(inSlide);
    if (theStoredRatio != -1)
        m_TimelineLayout->SetTimeRatio(theStoredRatio);
    else
        m_TimelineLayout->OnScalingReset();

    m_TimelineLayout->RecalcLayout();
}

void CTimelineControl::OnNewPresentation()
{
    m_TranslationManager->OnNewPresentation();

    // Register callback
    qt3dsdm::IStudioFullSystemSignalProvider *theSignalProvider =
        GetDoc()->GetStudioSystem()->GetFullSystemSignalProvider();
    m_Connections.push_back(theSignalProvider->ConnectActiveSlide(
        std::bind(&CTimelineControl::OnActiveSlide, this, std::placeholders::_3)));
    m_Connections.push_back(theSignalProvider->ConnectSlideDeleted(
        std::bind(&CTimelineControl::OnDeleteSlide, this, std::placeholders::_1)));
    CDispatch *theDispatch = g_StudioApp.GetCore()->GetDispatch();
    m_Connections.push_back(theDispatch->ConnectSelectionChange(
        std::bind(&CTimelineControl::OnSelectionChange, this, std::placeholders::_1)));
}

//=============================================================================
/**
 * Notification from the dispatch that the presentation is being closed.
 * This will clear all the objects from this presentation.
 */
void CTimelineControl::OnClosingPresentation()
{
    ClearView();
    m_TimelineLayout->ClearAllTimeRatios();
    m_BreadCrumbToolbar->RefreshTrail(nullptr);

    m_Connections.clear();
}

//=============================================================================
/**
 * Accessor for the root object being displayed in this view.
 */
qt3dsdm::CUICDMSlideHandle CTimelineControl::GetActiveSlide()
{
    return m_ActiveSlide;
}

//=============================================================================
/**
 * Gets the timeline layout which is the portion of the timeline to the right
 * of the splitter.  The timeline layout contains the timebars.
 */
CTimelineTimelineLayout *CTimelineControl::GetTimelineLayout()
{
    return m_TimelineLayout;
}

//=============================================================================
/**
 * Gets the tree layout which is the portion of the timeline to the left
 * of the splitter.  The tree layout contains the tree controls for expanding
 * rows in the timeline.
 */
CTimelineTreeLayout *CTimelineControl::GetTreeLayout()
{
    return m_TreeLayout;
}

//=============================================================================
/**
 * Notification from the dispatch that the presentation is going into play mode.
 */
void CTimelineControl::OnPlayStart()
{
}

//=============================================================================
/**
 * Notification from the dispatch that the presentation is exiting play state.
 */
void CTimelineControl::OnPlayStop()
{
}

//=============================================================================
/**
 * Notification from the dispatch that the time has changed.
 * This is used to update the playhead location and view time.
 * @param inNewTime the new time that this should display.
 */
void CTimelineControl::OnTimeChanged(long inNewTime)
{
    SetTime(inNewTime);
}

//==============================================================================
//	 CSelectionChangeListener
//==============================================================================
void CTimelineControl::OnSelectionChange(Q3DStudio::SSelectedValue inNewSelectable)
{
    // testing for nullptr selection OR if the selected is not displayed in the timeline
    bool theLoseFocus = !inNewSelectable.empty();
    if (!theLoseFocus) {
        Q3DStudio::SelectedValueTypes::Enum theSelectionType = inNewSelectable.getType();
        // for now, its just UICDM objects
        theLoseFocus = theSelectionType != Q3DStudio::SelectedValueTypes::Instance; // UICDM objects
    }
    if (theLoseFocus)
        m_TreeLayout->OnLoseFocus();

    GetTranslationManager()->OnSelectionChange(inNewSelectable);

    // The drag&drop doesn't have any sort of callback after a drop
    // so for now, this acts as a "event-trigger" after a drop ( because new items are always
    // selcted after a drop )
    HideInsertionMarkers();
}

//=============================================================================
/**
 * Callback when individual rows has affected the layout, such that the treelayout needs to be
 * synchronized with the timelinelayout or vice versa.
 */
void CTimelineControl::OnLayoutChanged()
{
    if (m_SuspendRecalcLayout) // optimization where this is explicitly shutoff.
        return;

    m_TreeLayout->RecalcLayout();
    m_TimelineLayout->OnTimelineLayoutChanged();
}

//=============================================================================
/**
 * typically for displaying tooltip
 */
CRct CTimelineControl::GetBounds() const
{
    return CRct(GetGlobalPosition(CPt(0, 0)), GetSize());
}

void CTimelineControl::HideTimelineMoveableTooltip()
{
    HideMoveableWindow();
}

//=============================================================================
/**
 * For snapping timebars/keyframes
 */
ISnappingListProvider *CTimelineControl::GetSnappingListProvider() const
{
    return m_TimelineLayout;
}

//=============================================================================
/**
 * Sets the current time as seen in this palette.
 * This will update the Playhead time and the time view time.
 * @param inNewTime the time to set on this.
 */
void CTimelineControl::SetTime(long inNewTime)
{
    m_TimelineLayout->SetTime(inNewTime);
    m_TreeLayout->SetTime(inNewTime);
}

void CTimelineControl::HideInsertionMarkers()
{
    bool theInvalidate = false;
    if (m_InsertionOverlay->IsVisible()) {
        m_InsertionOverlay->SetVisible(false);
        theInvalidate = true;
    }
    if (m_InsertionLine->IsVisible()) {
        m_InsertionLine->SetVisible(false);
        theInvalidate = true;
    }
    if (theInvalidate) {
        m_TreeLayout->Invalidate();
        Invalidate();
    }
}

void CTimelineControl::SetSize(CPt inSize)
{
    CControl::SetSize(inSize);

    m_Splitter->SetSize(CPt(inSize.x, inSize.y));
}

//=============================================================================
/**
 * Scrolls both sides of the timeline along the y-axis so that they stay synced.
 * @param inSource Scroller that generated the scroll messsage
 * @param inPositionY New vertical scroll bar position
 */
void CTimelineControl::SetScrollPositionY(CScroller *inSource, long inPositionY)
{
    m_TreeLayout->SetScrollPositionY(inSource, inPositionY);
    m_TimelineLayout->SetScrollPositionY(inSource, inPositionY);
}

//=============================================================================
/**
 * Override OnDraw to provide Timeline only draw profiling stats.
 */
void CTimelineControl::OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation)
{
    UICPROFILE(OnDraw);
    CControl::OnDraw(inRenderer, inDirtyRect, inIgnoreValidation);
}

//=============================================================================
/**
 * Fills the whole control with the base (gray) color, then other controls will
 * draw on top of that.
 * @param inRenderer renderer to draw to
 */
void CTimelineControl::Draw(CRenderer *inRenderer)
{
    const auto size = GetSize();
    inRenderer->FillSolidRect(QRect(0, 0, size.x, size.y), CStudioPreferences::GetBaseColor());
}

//=============================================================================
/**
 * Overriden from CControl. We want to propagate keydown (specifically F2)
 * messages to the selected row regardless if the control has focus or not.
 */
void CTimelineControl::OnGainFocus()
{
    CControl::OnGainFocus();

    CBaseStateRow *theRow = m_TranslationManager->GetSelectedRow();
    if (theRow)
        theRow->SetFocus();
}

//=============================================================================
/**
 * Overridden to draw insertion lines
 */
CDropTarget *CTimelineControl::FindDropCandidate(CPt &inMousePoint, Qt::KeyboardModifiers inFlags)
{
    CDropTarget *theDropTarget = CControl::FindDropCandidate(inMousePoint, inFlags);

    bool theHideInsertionMarkers = true;
    CTimeLineDropTarget *theTimelineDropTarget = nullptr;
    if (theDropTarget
        && (theTimelineDropTarget = dynamic_cast<CTimeLineDropTarget *>(theDropTarget))) {
        CControl *theInsertionOverControl = theTimelineDropTarget->GetInsertionMarkerRow();
        if (theInsertionOverControl) {
            CRct theTreeRect = GetVisibleTreeLayoutArea();
            EDROPDESTINATION theDropDest = theTimelineDropTarget->GetDestination();
            switch (theDropDest) {
            case EDROPDESTINATION_ABOVE:
            case EDROPDESTINATION_BELOW: {
                // the insertion line starts from the indent to the end of the row
                long theIndent = theTimelineDropTarget->GetInsertionMarkerIndent();
                if (theDropDest == EDROPDESTINATION_ABOVE)
                    m_InsertionLine->SetPosition(theInsertionOverControl->GetGlobalPosition(
                        CPt(theIndent, -GetPosition().y)));
                else
                    m_InsertionLine->SetPosition(theInsertionOverControl->GetGlobalPosition(CPt(
                        theIndent, theInsertionOverControl->GetSize().y - 1 - GetPosition().y)));

                long theWidth =
                    theTreeRect.size.x + theTreeRect.position.x - m_InsertionLine->GetPosition().x;
                m_InsertionLine->SetLineWidth(theWidth);
                m_InsertionLine->SetVisible(true);
                m_InsertionOverlay->SetVisible(false);
            } break;
            case EDROPDESTINATION_ON: {
                // insertion overlay spans the width of the row
                m_InsertionOverlay->SetPosition(theInsertionOverControl->GetGlobalPosition(
                    CPt(theTreeRect.position.x, -GetPosition().y)));

                long theWidth = theTreeRect.size.x + theTreeRect.position.x
                    - m_InsertionOverlay->GetPosition().x;
                m_InsertionOverlay->SetWidth(theWidth);
                m_InsertionOverlay->SetVisible(true);
                m_InsertionLine->SetVisible(false);
            } break;
            }
            theHideInsertionMarkers = false;
        }
    }
    // not drawn
    if (theHideInsertionMarkers)
        HideInsertionMarkers();

    return theDropTarget;
}

void CTimelineControl::OnMouseOut(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseOut(inPoint, inFlags);
    HideInsertionMarkers();
}

void CTimelineControl::OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    CControl::OnMouseUp(inPoint, inFlags);
    HideInsertionMarkers();
}

//=============================================================================
/**
 * Gets the insertion line for the timeline.  The insertion line should be used
 * to indicate when you can drag-and-drop and item between two other items.
 * Call SetVisible on this control to show/hide it.
 * @return the insertion line control
 */
CInsertionLine *CTimelineControl::GetInsertionLine()
{
    return m_InsertionLine;
}

//=============================================================================
/**
 * Gets the insertion overlay marker for the timeline.  This control should be
 * used to indicate that you can drag-and-drop and object onto another item in
 * the timeline.  Call SetVisible on this control to show/hide it.
 * @return the insertion overlay marker for the timeline
 */
CInsertionOverlay *CTimelineControl::GetInsertionOverlay()
{
    return m_InsertionOverlay;
}

//=============================================================================
/**
 * Fetches the bounding rect for the CTimelineTreeLayout section of the
 * timeline.  This is the section that contains toggles and text names of items
 * in the timeline.  The actual tree layout might be bigger than this rect
 * specifies.  This is because portions of the tree layout might be overlapped
 * by other controls.
 * @return rectangle describing visible area of the tree control
 */
CRct CTimelineControl::GetVisibleTreeLayoutArea()
{
    return m_TreeLayout->GetVisibleArea();
}

void CTimelineControl::RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler)
{
    m_TimelineLayout->RegisterGlobalKeyboardShortcuts(inShortcutHandler);
}

//=============================================================================
/**
 * event that takes place just before a save or export, on lose focus will commit changes
 * in text boxes
 */
void CTimelineControl::OnSavingPresentation(const CUICFile *inNewPresentationFile)
{
    Q_UNUSED(inNewPresentationFile);
    OnLoseFocus();
}

//=============================================================================
/**
 * Notification from the StudioFullSystem signal provider that a slide has been deleted.
 */
void CTimelineControl::OnDeleteSlide(qt3dsdm::CUICDMSlideHandle inSlide)
{
    m_TimelineLayout->DeleteTimelineRatio(inSlide);
}

//==============================================================================
/**
 * When caller knows that there are 'batch' changes to the timeline layout,
 * to prevent unnecessary calls to recalclayout
 */
void CTimelineControl::SuspendLayoutChanges(bool inSuspend)
{
    m_SuspendRecalcLayout = inSuspend;
}

CDoc *CTimelineControl::GetDoc()
{
    return g_StudioApp.GetCore()->GetDoc();
}

CClientDataModelBridge *CTimelineControl::GetBridge()
{
    return GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
}
