/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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

#include "KeyframesManager.h"
#include "IDoc.h"
#include "TimeEditDlg.h"
#include "TimelineTranslationManager.h"
#include "UICDMTimelineKeyframe.h"
#include "UICDMTimelineItemBinding.h"
#include "CmdDataModelRemoveKeyframe.h"
#include "CmdDataModelInsertKeyframe.h"
#include "CmdDataModelChangeKeyframe.h"
#include "UICDMAnimation.h"
#include "ClientDataModelBridge.h"
#include "PasteKeyframesCommandHelper.h"
#include "IDocumentEditor.h"
#include "IKeyframe.h"
#include "Dispatch.h"
#include "StudioPreferences.h"

#include "StudioApp.h" //for CommitCurrentCommand
#include "Core.h"
#include "Dialogs.h"

using namespace qt3dsdm;

bool SortKeyframeInstancePairByTime(const CKeyframesManager::SKeyframeInstancePair &inLHS,
                                    const CKeyframesManager::SKeyframeInstancePair &inRHS)
{
    return inLHS.m_Keyframe->GetTime() < inRHS.m_Keyframe->GetTime();
}

//==============================================================================
//	Keyframe specific.
//  UICDM selection is handled by CTimelineTranslationManager.
//==============================================================================
CKeyframesManager::CKeyframesManager(CTimelineTranslationManager *inTransMgr)
    : m_TransMgr(inTransMgr)
    , m_OffsetKeyframeCommandHelper(*(g_StudioApp.GetCore()->GetDoc()))
    , m_PasteKeyframeCommandHelper(nullptr)
{
    g_StudioApp.GetCore()->GetDoc()->SetKeyframesManager(this);
}

CKeyframesManager::~CKeyframesManager()
{
    delete m_PasteKeyframeCommandHelper;
}

bool CKeyframesManager::HasSelectedKeyframes(bool inOnlyDynamic /*= false */)
{
    // specifically only to know if there are any selected keyframes that are dynamic
    if (inOnlyDynamic) {
        for (size_t theIndex = 0; theIndex < m_SelectedKeyframes.size(); ++theIndex)
            if (m_SelectedKeyframes[theIndex].m_Keyframe->IsDynamic())
                return true;

        return false;
    }

    bool theRetVal = !m_SelectedKeyframes.empty();

    return theRetVal;
}

bool CKeyframesManager::CanPerformKeyframeCopy()
{
    bool theCanCopyNewData = false;
    // Legacy system actually prevents copy/pasting between different instances, so let's preserve
    // that
    if (!m_SelectedKeyframes.empty()) {
        theCanCopyNewData = true;
        if (m_SelectedKeyframes.size() > 1) {
            CUICDMTimelineItemBinding *theInstance = m_SelectedKeyframes[0].m_Instance;
            TSelectedKeyframeList::iterator theIter = m_SelectedKeyframes.begin();
            ++theIter;
            for (; theIter != m_SelectedKeyframes.end() && theCanCopyNewData; ++theIter) {
                if (theIter->m_Instance != theInstance) // fail!
                    theCanCopyNewData = false;
            }
        }
    }
    return theCanCopyNewData;
}

bool CKeyframesManager::CanPerformKeyframePaste()
{
    if (m_PasteKeyframeCommandHelper && m_PasteKeyframeCommandHelper->HasCopiedKeyframes()) {
        qt3dsdm::CUICDMInstanceHandle theSelectedInstance =
            g_StudioApp.GetCore()->GetDoc()->GetSelectedInstance();
        if (theSelectedInstance.Valid()) {
            return true;
        }
    }
    return false;
}

void CKeyframesManager::CopyKeyframes()
{
    CopySelectedKeyframes();
}

// legacy stuff that we have to support for animation tracks in the old data model to work
inline void PostExecuteCommand(IDoc *inDoc)
{
    CDoc *theDoc = dynamic_cast<CDoc *>(inDoc);
    theDoc->GetCore()->CommitCurrentCommand();
    // fire render event.
    // theDoc->UpdateClientScene( true );
}

//@param inPerformCopy true if that is a copy/cut command. false if this is a delete.
// Note: Keyframes are never explicitly copied to the clipboard (only as part of a asset copy),
// hence that means that the keyframes (only) are never copied across different instances of studio.
bool CKeyframesManager::RemoveKeyframes(bool inPerformCopy)
{
    bool theRetVal = HasSelectedKeyframes();

    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();

    if (inPerformCopy) // copy prior to removing the keyframes
        CopySelectedKeyframes();

    CCmdDataModelRemoveKeyframe *theCmd = nullptr;
    if (!m_SelectedKeyframes.empty()) {
        TSelectedKeyframeList::iterator theKeyIter = m_SelectedKeyframes.begin();
        for (; theKeyIter != m_SelectedKeyframes.end(); ++theKeyIter) {
            CUICDMTimelineKeyframe::TKeyframeHandleList theKeyframeHandles;
            theKeyIter->m_Keyframe->GetKeyframeHandles(theKeyframeHandles);
            ASSERT(!theKeyframeHandles.empty());
            CUICDMTimelineKeyframe::TKeyframeHandleList::iterator theIter =
                theKeyframeHandles.begin();
            if (!theCmd) {
                theCmd = new CCmdDataModelRemoveKeyframe(theDoc, *theIter);
                ++theIter;
            }
            for (; theIter != theKeyframeHandles.end(); ++theIter)
                theCmd->AddKeyframeHandle(*theIter);
        }
    }

    if (theCmd) {
        g_StudioApp.GetCore()->ExecuteCommand(theCmd);
        PostExecuteCommand(theDoc);
    }
    return theRetVal;
}

// note: we can't paste data from old data model system to the new one, and vice versa. so either
// the old Or the new system succeeds in pasting, never both.
void CKeyframesManager::PasteKeyframes()
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();

    if (m_PasteKeyframeCommandHelper && m_PasteKeyframeCommandHelper->HasCopiedKeyframes()) {
        qt3dsdm::CUICDMInstanceHandle theSelectedInstance = theDoc->GetSelectedInstance();
        if (theSelectedInstance.Valid()) {
            long theCurrentViewTimeInMilliseconds = theDoc->GetCurrentViewTime();
            CCmdDataModelInsertKeyframe *theInsertKeyframesCommand =
                m_PasteKeyframeCommandHelper->GetCommand(theDoc, theCurrentViewTimeInMilliseconds,
                                                         theSelectedInstance);
            if (theInsertKeyframesCommand)
                g_StudioApp.GetCore()->ExecuteCommand(theInsertKeyframesCommand);
        }
    }
}

//=============================================================================
/**
 * Sets interpolation values of all selected keyframes to the values specified
 * by the user.  Pops up a dialog prompting the user to choose new ease in and
 * ease out values.  Values in the dialog are initialized to the left-most
 * selected keyframe's interpolation values.
 */
void CKeyframesManager::SetKeyframeInterpolation()
{
    if (!HasSelectedKeyframes())
        return;

    float theEaseIn = 0;
    float theEaseOut = 0;
    if (CStudioPreferences::GetInterpolation())
        theEaseIn = theEaseOut = 100;

    IAnimationCore *theAnimationCore = m_TransMgr->GetStudioSystem()->GetAnimationCore();

    if (!m_SelectedKeyframes.empty()) // this is a sorted list, so we only need to grab tge ease
                                      // in/out values from the first item in this list.
    {
        CUICDMTimelineKeyframe *theTimelineKeyframe = m_SelectedKeyframes.front().m_Keyframe;
        CUICDMTimelineKeyframe::TKeyframeHandleList theKeyframeHandles;
        theTimelineKeyframe->GetKeyframeHandles(theKeyframeHandles);
        TKeyframe theKeyframeData = theAnimationCore->GetKeyframeData(theKeyframeHandles[0]);
        GetEaseInOutValues(theKeyframeData, theEaseIn, theEaseOut);
    }

    if (g_StudioApp.GetDialogs()->PromptForKeyframeInterpolation(theEaseIn, theEaseOut)) {
        CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
        Q3DStudio::ScopedDocumentEditor editor(*theDoc, L"Set Keyframe Interpolation", __FILE__,
                                               __LINE__);
        TSelectedKeyframeList::iterator theKeyIter = m_SelectedKeyframes.begin();
        for (; theKeyIter != m_SelectedKeyframes.end(); ++theKeyIter) {
            CUICDMTimelineKeyframe *theTimelineKeyframe = theKeyIter->m_Keyframe;
            CUICDMTimelineKeyframe::TKeyframeHandleList theKeyframeHandles;
            theTimelineKeyframe->GetKeyframeHandles(theKeyframeHandles);
            for (size_t i = 0; i < theKeyframeHandles.size(); ++i) {
                TKeyframe theKeyframeData =
                    theAnimationCore->GetKeyframeData(theKeyframeHandles[i]);
                SetEaseInOutValues(theKeyframeData, theEaseIn, theEaseOut);
                theAnimationCore->SetKeyframeData(theKeyframeHandles[i], theKeyframeData);
            }
        }
    }
}

bool CKeyframesManager::HasDynamicKeyframes()
{
    CUICDMTimelineItemBinding *theBinding = m_TransMgr->GetSelectedBinding();
    if (theBinding) {
        return theBinding->HasDynamicKeyframes(-1);
    }
    return false;
}

void CKeyframesManager::SelectAllKeyframes()
{
    ITimelineItemBinding *theBinding = m_TransMgr->GetSelectedBinding();
    if (theBinding)
        theBinding->SelectKeyframes(true);
}

void CKeyframesManager::DeselectAllKeyframes()
{
    m_TransMgr->ClearBindingsKeyframeSelection();
    m_SelectedKeyframes.clear();
}

//==============================================================================
/**
 * Sets keyframes on all the changed properties of the selected object.
 * Also known as autoset keyframes, but it only applies to one object, and is
 * the result of an F6 key press.  Tells the TimelineCtrl to autoset keyframes.
 */
void CKeyframesManager::SetChangedKeyframes()
{

    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::CUICDMInstanceHandle theSelectedInstance = theDoc->GetSelectedInstance();
    if (theSelectedInstance.Valid()) {
        using namespace Q3DStudio;
        Q3DStudio::ScopedDocumentEditor editor(*theDoc, L"Set Changed Keyframes", __FILE__,
                                               __LINE__);
        CStudioSystem *theStudioSystem = theDoc->GetStudioSystem();
        // Get all animated properties.
        TPropertyHandleList theProperties;
        theStudioSystem->GetPropertySystem()->GetAggregateInstanceProperties(theSelectedInstance,
                                                                             theProperties);
        for (size_t thePropertyIndex = 0; thePropertyIndex < theProperties.size();
             ++thePropertyIndex) {
            if (theStudioSystem->GetAnimationSystem()->IsPropertyAnimated(
                    theSelectedInstance, theProperties[thePropertyIndex]))
                editor->KeyframeProperty(theSelectedInstance, theProperties[thePropertyIndex],
                                         true);
        }
    }
}

void CKeyframesManager::SetKeyframeTime(long inTime)
{
    CTimeEditDlg theTimeEditDlg;
    theTimeEditDlg.SetKeyframesManager(this);
    theTimeEditDlg.ShowDialog(inTime, 0, g_StudioApp.GetCore()->GetDoc(), ASSETKEYFRAME);
}

void CKeyframesManager::SetKeyframeDynamic(CUICDMTimelineKeyframe *inKeyframe, bool inDynamic)
{
    CDoc *theDoc = g_StudioApp.GetCore()->GetDoc();
    qt3dsdm::TKeyframeHandleList theKeyframeHandle;
    CCmdDataModelChangeDynamicKeyframe *theCmd = nullptr;

    if (inKeyframe != nullptr)
        inKeyframe->GetKeyframeHandles(theKeyframeHandle);

    qt3dsdm::IAnimationCore *theAnimationCore = theDoc->GetStudioSystem()->GetAnimationCore();
    for (size_t theKeyframe = 0; theKeyframe < theKeyframeHandle.size(); ++theKeyframe) {
        qt3dsdm::CUICDMAnimationHandle theAnimation(
            theAnimationCore->GetAnimationForKeyframe(theKeyframeHandle.at(theKeyframe)));
        if (!theCmd)
            theCmd = new CCmdDataModelChangeDynamicKeyframe(theDoc, theAnimation, inDynamic);
        else
            theCmd->AddHandle(theAnimation);
    }

    if (theCmd)
        g_StudioApp.GetCore()->ExecuteCommand(theCmd);
}

void CKeyframesManager::SetKeyframesDynamic(bool inDynamic)
{

    CUICDMTimelineKeyframe *theKeyframe;

    if (m_SelectedKeyframes.size() == 0) {
        CUICDMTimelineItemBinding *theBinding = m_TransMgr->GetSelectedBinding();
        IKeyframe *key = theBinding->GetKeyframeByIndex(0);
        theKeyframe = dynamic_cast<CUICDMTimelineKeyframe *>(key);
        SetKeyframeDynamic(theKeyframe, inDynamic);
    } else {
        for (int i = 0; i < (int)m_SelectedKeyframes.size(); ++i) {
            theKeyframe = m_SelectedKeyframes[i].m_Keyframe;
            SetKeyframeDynamic(theKeyframe, inDynamic);
        }
    }
}

long CKeyframesManager::OffsetSelectedKeyframes(long inOffset)
{
    m_InstanceSet.clear();
    m_InstanceList.clear();
    std::set<CUICDMTimelineItemBinding *> &theInstances(m_InstanceSet);

    TSelectedKeyframeList::iterator theKeyIter = m_SelectedKeyframes.begin();
    for (; theKeyIter != m_SelectedKeyframes.end(); ++theKeyIter) {
        // since this list is sorted by time and we are iterating from the first in the list
        long theKeyframeTime = theKeyIter->m_Keyframe->GetTime();
        if (inOffset < 0 && theKeyframeTime + inOffset < 0)
            inOffset = -theKeyframeTime;

        theKeyIter->m_Keyframe->UpdateKeyframesTime(&m_OffsetKeyframeCommandHelper,
                                                    theKeyframeTime + inOffset);

        // this contains unique instancs, i.e. mulitple keyframe can map to the same instances
        if (theInstances.insert(theKeyIter->m_Instance).second)
            m_InstanceList.push_back(theKeyIter->m_Instance->GetInstance());
    }

    // UI update, explicitly because this doesn't generate any events till action is committed
    std::set<CUICDMTimelineItemBinding *>::iterator theInstanceIter = theInstances.begin();
    for (; theInstanceIter != theInstances.end(); ++theInstanceIter)
        (*theInstanceIter)->UIRefreshPropertyKeyframe(inOffset);

    if (m_InstanceList.size())
        m_TransMgr->GetDoc()->GetCore()->GetDispatch()->FireImmediateRefreshInstance(
            &m_InstanceList[0], (long)m_InstanceList.size());

    // by contract, this functions returns the "legal" offset, ie time cannot be offset to negative.
    return inOffset;
}

void CKeyframesManager::CommitChangedKeyframes()
{
    m_OffsetKeyframeCommandHelper.Finalize();
}

void CKeyframesManager::RollbackChangedKeyframes()
{
    m_OffsetKeyframeCommandHelper.Rollback();
}

bool CKeyframesManager::CanMakeSelectedKeyframesDynamic()
{
    using namespace qt3dsdm;
    TKeyframeHandleList theKeyframes;
    TKeyframeHandleList allTheKeyframes;
    IAnimationCore &theAnimationCore(
        *g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetAnimationCore());
    // Ensure that all keyframes selected are the first keyframes from the animation track.
    for (size_t idx = 0, end = m_SelectedKeyframes.size(); idx < end; ++idx) {
        theKeyframes.clear();
        m_SelectedKeyframes.at(idx).m_Keyframe->GetKeyframeHandles(theKeyframes);
        for (size_t specificKeyframeIdx = 0, specificKeyframeEnd = theKeyframes.size();
             specificKeyframeIdx < specificKeyframeEnd; ++specificKeyframeIdx) {
            CUICDMKeyframeHandle theKeyframe = theKeyframes[specificKeyframeIdx];
            CUICDMAnimationHandle theAnimation =
                theAnimationCore.GetAnimationForKeyframe(theKeyframe);
            allTheKeyframes.clear();
            theAnimationCore.GetKeyframes(theAnimation, allTheKeyframes);
            if (allTheKeyframes[0] != theKeyframe)
                return false;
        }
    }
    return true;
}

// keeps track of selected keyframes so that we don't have to iterate through the entire hierarchy
// to find them
void CKeyframesManager::SetKeyframeSelected(CUICDMTimelineKeyframe *inKeyframe, bool inSelected,
                                            CUICDMTimelineItemBinding *inOwningInstance /*= nullptr */)
{
    TSelectedKeyframeList::iterator theKeyIter = m_SelectedKeyframes.begin();
    for (; theKeyIter != m_SelectedKeyframes.end(); ++theKeyIter) {
        if (theKeyIter->m_Keyframe == inKeyframe)
            break;
    }
    if (inSelected) {
        ASSERT(inOwningInstance);
        if (theKeyIter == m_SelectedKeyframes.end()) { // only this is not already selected
            m_SelectedKeyframes.push_back(SKeyframeInstancePair(inKeyframe, inOwningInstance));
            std::sort(m_SelectedKeyframes.begin(), m_SelectedKeyframes.end(),
                      SortKeyframeInstancePairByTime);
        }
    } else if (theKeyIter != m_SelectedKeyframes.end()) {
        m_SelectedKeyframes.erase(theKeyIter);
    }
}

qt3dsdm::SGetOrSetKeyframeInfo SetupKeyframeInfo(qt3dsdm::CUICDMKeyframeHandle inKeyframe,
                                               qt3dsdm::IAnimationCore &inCore)
{
    TKeyframe theKeyframeData = inCore.GetKeyframeData(inKeyframe);
    SEaseInEaseOutKeyframe theKeyframe = qt3dsdm::get<SEaseInEaseOutKeyframe>(theKeyframeData);
    // Is this the first keyframe?
    bool isDynamic = false;
    if (inCore.IsFirstKeyframe(inKeyframe))
        isDynamic = inCore.GetAnimationInfo(inCore.GetAnimationForKeyframe(inKeyframe))
                        .m_DynamicFirstKeyframe;

    return SGetOrSetKeyframeInfo(theKeyframe.m_KeyframeValue, theKeyframe.m_EaseIn,
                                 theKeyframe.m_EaseOut, isDynamic);
}

// only deal with this manager's selected keyframes
void CKeyframesManager::CopySelectedKeyframes()
{
    if (!m_SelectedKeyframes.empty()) {
        if (m_PasteKeyframeCommandHelper)
            m_PasteKeyframeCommandHelper->Clear(); // clear out previously copied data
        else
            m_PasteKeyframeCommandHelper = new CPasteKeyframeCommandHelper();

        // note: m_SelectedKeyframes is already sorted by time
        float theEarliestKeyframeTimeInSecs =
            CUICDMTimelineKeyframe::GetTimeInSecs(m_SelectedKeyframes[0].m_Keyframe->GetTime());

        IAnimationCore *theAnimationCore = m_TransMgr->GetStudioSystem()->GetAnimationCore();
        TSelectedKeyframeList::iterator theIter = m_SelectedKeyframes.begin();
        for (; theIter != m_SelectedKeyframes.end(); ++theIter) {
            CUICDMTimelineKeyframe *theKeyframe = (*theIter).m_Keyframe;
            CUICDMTimelineKeyframe::TKeyframeHandleList theKeyframeHandles;
            theKeyframe->GetKeyframeHandles(theKeyframeHandles);
            qt3dsdm::SGetOrSetKeyframeInfo theInfos[3];
            size_t theValidInfos = 0;
            if (!theKeyframeHandles.empty()) {
                // TODO: need to figure out a good way to convert from individual keyframes back to
                // SValue
                SValue theValue;
                switch (theKeyframeHandles.size()) {
                case 1: {
                    theInfos[0] = SetupKeyframeInfo(theKeyframeHandles[0], *theAnimationCore);
                    theValidInfos = 1;

                } break;
                case 3: {
                    theInfos[0] = SetupKeyframeInfo(theKeyframeHandles[0], *theAnimationCore);
                    theInfos[1] = SetupKeyframeInfo(theKeyframeHandles[1], *theAnimationCore);
                    theInfos[2] = SetupKeyframeInfo(theKeyframeHandles[2], *theAnimationCore);
                    theValidInfos = 3;
                } break;
                default: // not handled
                    break;
                }
                // time is relative to the earliest keyframe time.
                float theRelativeTimeInSecs =
                    CUICDMTimelineKeyframe::GetTimeInSecs(theKeyframe->GetTime())
                    - theEarliestKeyframeTimeInSecs;

                CUICDMAnimationHandle theAnimation =
                    theAnimationCore->GetAnimationForKeyframe(theKeyframeHandles[0]);
                m_PasteKeyframeCommandHelper->AddKeyframeData(
                    theAnimationCore->GetAnimationInfo(theAnimation).m_Property,
                    theRelativeTimeInSecs, theInfos, theValidInfos);
            }
        }
    }
}
