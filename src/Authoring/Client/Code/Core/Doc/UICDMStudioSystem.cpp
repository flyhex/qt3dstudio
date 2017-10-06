/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "stdafx.h"
#include "Doc.h"
#undef placeholder
#include <boost/function.hpp>
#include <boost/signal.hpp>
#include <boost/signals/connection.hpp>
#include "UICDMStudioSystem.h"
#include "DataCoreProducer.h"
#include "SlideCoreProducer.h"
#include "SlideGraphCoreProducer.h"
#include "AnimationCoreProducer.h"
#include "SlideSystem.h"
#include "ClientDataModelBridge.h"
#include "StudioCoreSystem.h"
#include "StudioFullSystem.h"
#include "UICDMXML.h"
#include "foundation/IOStreams.h"
#include "StudioUtils.h"

using namespace std;

namespace UICDM {
inline std::wstring GetFullPath(Q3DStudio::CString inSource)
{
#ifdef Q_OS_MACOS
    if (inSource.Left(4) == "res/" || inSource.Left(4) == "res\\")
        inSource = inSource.Mid(4);
#endif
    CUICFile theSourcePath(CUICFile::GetApplicationDirectory().GetAbsolutePath() + inSource);
    return std::wstring(theSourcePath.GetAbsolutePath());
}

CStudioSystem::CStudioSystem(CDoc *inDoc)
    : m_Doc(inDoc)
{
    ResetDatabase();
}

struct MaterialImagePropertyInfo : public IPropertyInstanceInfo
{
    std::shared_ptr<CClientDataModelBridge> m_Bridge;
    CUICDMPropertyHandle m_Slot;
    TDataCorePtr m_DataCore;
    TSlideCorePtr m_SlideCore;
    TAnimationCorePtr m_AnimationCore;
    MaterialImagePropertyInfo(std::shared_ptr<CClientDataModelBridge> inBridge,
                              CUICDMPropertyHandle inSlot, TDataCorePtr inDataCore,
                              TSlideCorePtr inSlideCore, TAnimationCorePtr inAnimationCore)
        : m_Bridge(inBridge)
        , m_Slot(inSlot)
        , m_DataCore(inDataCore)
        , m_SlideCore(inSlideCore)
        , m_AnimationCore(inAnimationCore)
    {
    }
    /**
     *	Return the instance that relates to this property
     */
    CUICDMInstanceHandle GetInstanceForProperty(const SValue &inValue) override
    {
        SLong4 theId(get<SLong4>(inValue));
        if (theId.m_Longs[0] && theId.m_Longs[1] && theId.m_Longs[2] && theId.m_Longs[3]) {
            CUICDMInstanceHandle theImageInstance = m_Bridge->GetImageInstanceByGUID(theId);
            return theImageInstance;
        }
        return 0;
    }

    static inline void SetPropertyIfOnInstance(const TSlideEntry &inEntry,
                                               CUICDMInstanceHandle inInstance,
                                               CUICDMInstanceHandle inDestInstance,
                                               CUICDMSlideHandle inDestSlide,
                                               TSlideCorePtr inSlideCore)
    {
        if (get<0>(inEntry) == inInstance)
            inSlideCore->ForceSetInstancePropertyValue(inDestSlide, inDestInstance, get<1>(inEntry),
                                                       get<2>(inEntry));
    }

    static inline void
    CopyAnimationIfOnInstance(CUICDMAnimationHandle inAnimation, CUICDMSlideHandle inSourceSlide,
                              CUICDMInstanceHandle inInstance, CUICDMInstanceHandle inDestInstance,
                              CUICDMSlideHandle inDestSlide, TAnimationCorePtr inAnimationCore)
    {
        SAnimationInfo theInfo(inAnimationCore->GetAnimationInfo(inAnimation));
        if (theInfo.m_Instance == inInstance && theInfo.m_Slide == inSourceSlide)
            CopyAnimation(inAnimationCore, inAnimation, inDestSlide, inDestInstance,
                          theInfo.m_Property, theInfo.m_Index);
    }

    /**
     *	Duplicate this instance and whichever properties and animations you desire,
     *	returning a new data model value that will be set on the newly created property.
     */
    SValue CreateInstanceForProperty(CUICDMSlideHandle inSourceSlide,
                                             CUICDMSlideHandle inDestSlide,
                                             CUICDMInstanceHandle inInstance) override
    {
        std::pair<UICDM::CUICDMInstanceHandle, UICDM::SLong4> theInstanceGuidPair(
            m_Bridge->CreateImageInstance(inInstance, m_Slot, inDestSlide));
        CUICDMInstanceHandle theNewInstance(theInstanceGuidPair.first);
        TSlideEntryList theEntries;
        m_SlideCore->GetSlidePropertyEntries(inSourceSlide, theEntries);
        do_all(theEntries, std::bind(SetPropertyIfOnInstance, std::placeholders::_1,
                                     inInstance, theNewInstance, inDestSlide, m_SlideCore));
        TAnimationHandleList theAnimations;
        m_AnimationCore->GetAnimations(theAnimations);
        do_all(theAnimations, std::bind(CopyAnimationIfOnInstance, std::placeholders::_1,
                                        inSourceSlide, inInstance, theNewInstance, inDestSlide,
                                        m_AnimationCore));
        return theInstanceGuidPair.second;
    }
};
// Call before load.
void CStudioSystem::ResetDatabase()
{
    std::shared_ptr<CStudioCoreSystem> theCore(new CStudioCoreSystem());
    // Create the base object model studio *has* to have to survive.
    std::shared_ptr<SComposerObjectDefinitions> theDefinitions =
        std::make_shared<SComposerObjectDefinitions>(std::ref(*theCore->GetDataCore()),
                                                       std::ref(*theCore->GetNewMetaData()));

    m_Bridge = std::shared_ptr<CClientDataModelBridge>(new CClientDataModelBridge(
        theCore->GetDataCore().get(), theCore->GetSlideCore().get(),
        theCore->GetSlideGraphCore().get(), theCore->GetAnimationCore().get(),
        theCore->GetNewMetaData(), theDefinitions, m_Doc));
    TNewMetaDataPtr theNewMetaData(theCore->GetNewMetaData());
    std::shared_ptr<IStringTable> theStringTable(theNewMetaData->GetStringTablePtr());
    std::shared_ptr<IDOMFactory> theFactory(IDOMFactory::CreateDOMFactory(theStringTable));
    Q3DStudio::CString theFullPath(Q3DStudio::CString::fromQString(
                                       resourcePath()
                                       + QStringLiteral("/DataModelMetadata/en-us/MetaData.xml")));
    // Load the new meta data
    {
        qt3ds::foundation::CFileSeekableIOStream theStream(theFullPath,
                                                           qt3ds::foundation::FileReadFlags());
        if (theStream.IsOpen() == false) {
            QT3DS_ASSERT(false);
            return;
        }
        SDOMElement *theElement = CDOMSerializer::Read(*theFactory, theStream);
        std::shared_ptr<IDOMReader> theReader =
            IDOMReader::CreateDOMReader(*theElement, theStringTable);
        theNewMetaData->Load(*theReader);
    }

    /*
    //Save new version of new meta data
    {
            SDOMElement* newTopElement = theFactory->NextElement( L"MetaData" );
            std::shared_ptr<IDOMWriter> theWriter( IDOMWriter::CreateDOMWriter( theFactory,
    *newTopElement, theStringTable ).first );
            theNewMetaData->Save( *theWriter);
            CFileSeekableIOStream theStream( theFullPath.GetCharStar(), FileWriteFlags() );
            CDOMSerializer::WriteXMLHeader( theStream );
            CDOMSerializer::Write( *newTopElement, theStream );
    }*/

    m_StudioSystem = std::shared_ptr<CStudioFullSystem>(new CStudioFullSystem(
        theCore, m_Bridge->GetSlideInstance(), m_Bridge->GetSlideComponentIdProperty(),
        m_Bridge->GetActionInstance(), m_Bridge->GetActionEyeball()));
}

IMetaData *CStudioSystem::GetActionMetaData()
{
    return m_StudioSystem->GetCoreSystem()->GetNewMetaData().get();
}

ISlideSystem *CStudioSystem::GetSlideSystem()
{
    return m_StudioSystem->GetSlideSystem().get();
}

ISlideCore *CStudioSystem::GetSlideCore()
{
    return m_StudioSystem->GetSlideCore().get();
}

IPropertySystem *CStudioSystem::GetPropertySystem()
{
    return m_StudioSystem->GetPropertySystem().get();
}
IStudioFullSystemSignalProvider *CStudioSystem::GetFullSystemSignalProvider()
{
    return m_StudioSystem->GetSignalProvider();
}
IAnimationCore *CStudioSystem::GetAnimationCore()
{
    return m_StudioSystem->GetAnimationCore().get();
}
IStudioAnimationSystem *CStudioSystem::GetAnimationSystem()
{
    return m_StudioSystem->GetAnimationSystem().get();
}
IActionCore *CStudioSystem::GetActionCore()
{
    return m_StudioSystem->GetActionCore().get();
}
IActionSystem *CStudioSystem::GetActionSystem()
{
    return m_StudioSystem->GetActionSystem().get();
}

void CStudioSystem::SetConsumer(TTransactionConsumerPtr inConsumer)
{
    m_StudioSystem->SetConsumer(inConsumer);
}

bool CStudioSystem::IsInstance(CUICDMInstanceHandle inInstance) const
{
    return m_StudioSystem->GetCoreSystem()->GetDataCore()->IsInstance(inInstance);
}
}
