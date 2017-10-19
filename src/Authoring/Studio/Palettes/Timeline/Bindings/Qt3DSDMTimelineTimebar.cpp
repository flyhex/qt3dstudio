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

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDMTimelineTimebar.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMDataTypes.h"
#include "ClientDataModelBridge.h"
#include "TimelineTranslationManager.h"
#include "Doc.h"
#include "Dispatch.h"
#include "Core.h"
#include "TimeEditDlg.h"
#include "IDocumentEditor.h"
#include "BaseStateRow.h"
#include "BaseTimebarlessRow.h"
#include "StudioFullSystem.h"
#include "StudioPreferences.h"
#include "ITimelineItemBinding.h"

CUICDMTimelineTimebar::CUICDMTimelineTimebar(
    CTimelineTranslationManager *inTimelineTranslationManager,
    qt3dsdm::Qt3DSDMInstanceHandle inDataHandle)
    : Q3DStudio::CUpdateableDocumentEditor(*inTimelineTranslationManager->GetDoc())
    , m_TimelineTranslationManager(inTimelineTranslationManager)
    , m_PropertySystem(inTimelineTranslationManager->GetStudioSystem()->GetPropertySystem())
    , m_DataHandle(inDataHandle)
{
    CClientDataModelBridge *theClientDataModelBridge =
        inTimelineTranslationManager->GetStudioSystem()->GetClientDataModelBridge();
    m_StartTime = theClientDataModelBridge->GetSceneAsset().m_StartTime;
    m_EndTime = theClientDataModelBridge->GetSceneAsset().m_EndTime;
    qt3dsdm::SValue theValue;
    if (m_PropertySystem->GetInstancePropertyValue(
            m_DataHandle, theClientDataModelBridge->GetSceneAsset().m_TimebarColor, theValue)) {
        qt3dsdm::SFloat3 theTimebarColor = qt3dsdm::get<qt3dsdm::SFloat3>(theValue);

        m_Color.SetRGB(static_cast<int>(theTimebarColor.m_Floats[0] * 255.0f),
                       static_cast<int>(theTimebarColor.m_Floats[1] * 255.0f),
                       static_cast<int>(theTimebarColor.m_Floats[2] * 255.0f));
    }
    qt3dsdm::IStudioFullSystemSignalProvider *theProvider =
        inTimelineTranslationManager->GetStudioSystem()->GetFullSystem()->GetSignalProvider();
    m_PropertyChangedSignal = theProvider->ConnectInstancePropertyValue(
        std::bind(&CUICDMTimelineTimebar::OnPropertyChanged, this,
                  std::placeholders::_1, std::placeholders::_2));

    OnPropertyChanged(m_DataHandle, theClientDataModelBridge->GetSceneAsset().m_TimebarColor);
    OnPropertyChanged(m_DataHandle, theClientDataModelBridge->GetSceneAsset().m_TimebarText);
}

void CUICDMTimelineTimebar::OnPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                              qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    if (m_DataHandle == inInstance) {
        bool needsInvalidate = false;
        qt3dsdm::SValue theValue;
        CClientDataModelBridge *theClientDataModelBridge =
            m_TimelineTranslationManager->GetStudioSystem()->GetClientDataModelBridge();
        if (inProperty == theClientDataModelBridge->GetSceneAsset().m_TimebarColor) {

            if (m_PropertySystem->GetInstancePropertyValue(
                    m_DataHandle, theClientDataModelBridge->GetSceneAsset().m_TimebarColor,
                    theValue)) {
                qt3dsdm::SFloat3 theTimebarColor = qt3dsdm::get<qt3dsdm::SFloat3>(theValue);

                m_Color.SetRGB(static_cast<int>(theTimebarColor.m_Floats[0] * 255.0f),
                               static_cast<int>(theTimebarColor.m_Floats[1] * 255.0f),
                               static_cast<int>(theTimebarColor.m_Floats[2] * 255.0f));
            } else {
                switch (theClientDataModelBridge->GetObjectType(inInstance)) {
                case OBJTYPE_LAYER:
                    m_Color = CStudioPreferences::GetLayerTimebarColor();
                    break;
                case OBJTYPE_BEHAVIOR:
                    m_Color = CStudioPreferences::GetBehaviorTimebarColor();
                    break;
                case OBJTYPE_CAMERA:
                    m_Color = CStudioPreferences::GetCameraTimebarColor();
                    break;
                case OBJTYPE_LIGHT:
                    m_Color = CStudioPreferences::GetLightTimebarColor();
                    break;
                case OBJTYPE_MODEL:
                    m_Color = CStudioPreferences::GetModelTimebarColor();
                    break;
                case OBJTYPE_GROUP:
                    m_Color = CStudioPreferences::GetGroupTimebarColor();
                    break;
                case OBJTYPE_COMPONENT:
                    m_Color = CStudioPreferences::GetComponentTimebarColor();
                    break;
                case OBJTYPE_EFFECT:
                    m_Color = CStudioPreferences::GetEffectTimebarColor();
                    break;
                default:
                    m_Color = CStudioPreferences::GetObjectTimebarColor();
                    break;
                }
            }
            needsInvalidate = true;
        } else if (inProperty == theClientDataModelBridge->GetSceneAsset().m_TimebarText) {
            if (m_PropertySystem->GetInstancePropertyValue(
                    m_DataHandle, theClientDataModelBridge->GetSceneAsset().m_TimebarText,
                    theValue)) {
                qt3dsdm::SStringRef theTimebarComment = qt3dsdm::get<qt3dsdm::SStringRef>(theValue);
                m_Comment.Assign(static_cast<const wchar_t *>(theTimebarComment));
            } else {
                m_Comment.Assign(L"");
            }
            needsInvalidate = true;
        }
        if (needsInvalidate) {
            ITimelineItemBinding *theBinding =
                m_TimelineTranslationManager->GetOrCreate(inInstance);
            if (theBinding) {
                CBaseStateRow *theRow = theBinding->GetRow();
                if (theRow) {
                    CBaseTimebarlessRow *theTimebar = theRow->GetTimebar();
                    theTimebar->RefreshRowMetaData();
                }
            }
        }
    }
}

CUICDMTimelineTimebar::~CUICDMTimelineTimebar()
{
}

// TODO: Can we put this on IInstancePropertyCore?
template <typename T>
T GetInstancePropertyValue(qt3dsdm::IPropertySystem *inPropertySystem,
                           qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty)
{
    qt3dsdm::SValue theValue;
    inPropertySystem->GetInstancePropertyValue(inInstanceHandle, inProperty, theValue);
    return qt3dsdm::get<T>(theValue);
}

long CUICDMTimelineTimebar::GetStartTime() const
{
    return GetInstancePropertyValue<qt3ds::QT3DSI32>(m_PropertySystem, m_DataHandle, m_StartTime);
}

long CUICDMTimelineTimebar::GetEndTime() const
{
    return GetInstancePropertyValue<qt3ds::QT3DSI32>(m_PropertySystem, m_DataHandle, m_EndTime);
}

long CUICDMTimelineTimebar::GetDuration() const
{
    auto theStartTime = GetInstancePropertyValue<qt3ds::QT3DSI32>(m_PropertySystem, m_DataHandle, m_StartTime);
    auto theEndTime = GetInstancePropertyValue<qt3ds::QT3DSI32>(m_PropertySystem, m_DataHandle, m_EndTime);

    return theEndTime - theStartTime;
}

bool CUICDMTimelineTimebar::ShowHandleBars() const
{
    return true;
}

void CUICDMTimelineTimebar::OnBeginDrag()
{ // Really? TODO: Figure out why this is here.
    // ASSERT(0);
}

void CUICDMTimelineTimebar::OffsetTime(long inDiff)
{
    if (m_DataHandle.Valid()) {
        ENSURE_EDITOR(L"Time Bar Move").OffsetTimeRange(m_DataHandle, inDiff);
        m_TimelineTranslationManager->GetDoc()
            ->GetCore()
            ->GetDispatch()
            ->FireImmediateRefreshInstance(m_DataHandle);
    }
}

void CUICDMTimelineTimebar::ChangeTime(long inTime, bool inSetStart)
{
    if (m_DataHandle.Valid()) {
        ENSURE_EDITOR(L"Time Bar Resize").ResizeTimeRange(m_DataHandle, inTime, inSetStart);
        m_TimelineTranslationManager->GetDoc()
            ->GetCore()
            ->GetDispatch()
            ->FireImmediateRefreshInstance(m_DataHandle);
    }
}

void CUICDMTimelineTimebar::CommitTimeChange()
{
    CommitEditor();
}

void CUICDMTimelineTimebar::RollbackTimeChange()
{
    RollbackEditor();
}

void CUICDMTimelineTimebar::SetTimebarColor(const ::CColor &inColor)
{
    using namespace Q3DStudio;
    if (inColor != m_Color) {
        qt3dsdm::Qt3DSDMInstanceHandle theHandle = m_DataHandle;
        SCOPED_DOCUMENT_EDITOR(*m_TimelineTranslationManager->GetDoc(), QObject::tr("Set Timebar Color"))
            ->SetTimebarColor(theHandle, inColor);
    }
}

void CUICDMTimelineTimebar::SetTimebarComment(const Q3DStudio::CString &inComment)
{
    using namespace Q3DStudio;
    if (inComment != m_Comment) {
        qt3dsdm::Qt3DSDMInstanceHandle theHandle = m_DataHandle;
        SCOPED_DOCUMENT_EDITOR(*m_TimelineTranslationManager->GetDoc(), QObject::tr("Set Timebar Text"))
            ->SetTimebarText(theHandle, inComment);
    }
}

void CUICDMTimelineTimebar::SetTimebarTime(ITimeChangeCallback *inCallback /*= nullptr*/)
{
    long theStartTime = GetStartTime();
    long theEndTime = GetEndTime();
    CTimeEditDlg theTimeEditDlg;
    theTimeEditDlg.ShowDialog(theStartTime, theEndTime, m_TimelineTranslationManager->GetDoc(),
                              TIMEBAR, inCallback);
}
