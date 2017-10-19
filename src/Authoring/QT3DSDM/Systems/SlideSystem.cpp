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
#include "Qt3DSDMPrefix.h"
#ifdef WIN32
#pragma warning(disable : 4503)
#endif
#include "SlideSystem.h"
#include "SimpleSlideCore.h"

using namespace std;

namespace qt3dsdm {

SSlideSystem::SSlideSystem(TDataCorePtr inDataCore, TSlideCorePtr inSlideCore,
                           TSlideGraphCorePtr inSlideGraphCore, TAnimationCorePtr inAnimationCore,
                           Qt3DSDMInstanceHandle inSlideInstance,
                           Qt3DSDMPropertyHandle inComponentGuidProperty)
    : m_DataCore(inDataCore)
    , m_SlideCore(inSlideCore)
    , m_SlideGraphCore(inSlideGraphCore)
    , m_AnimationCore(inAnimationCore)
    , m_SlideInstance(inSlideInstance)
    , m_ComponentGuid(inComponentGuidProperty)
{
    m_Signaller = CreateSlideSystemSignaller();
}
void SSlideSystem::SetPropertySystem(TPropertySystemPtr inPropertySystem)
{
    m_PropertySystem = inPropertySystem;
}

CUICDMSlideHandle SSlideSystem::CreateMasterSlide()
{
    Qt3DSDMInstanceHandle theSlideInstance = m_DataCore->CreateInstance();
    m_DataCore->DeriveInstance(theSlideInstance, m_SlideInstance);
    CUICDMSlideHandle retval = m_SlideCore->CreateSlide(theSlideInstance);
    m_SlideGraphCore->CreateSlideGraph(retval);
    GetSignalSender()->SendMasterCreated(retval);
    return retval;
}

inline bool PropertyHandlePairEquals(const TPropertyHandlePropertyInfoPair &inPair,
                                     Qt3DSDMPropertyHandle inProperty)
{
    return (inProperty == inPair.first);
}

void AddReferencedInstance(const TSlideEntry &inEntry,
                           const TPropertyHandlePropertyInfoPairList &inInfoPairList,
                           CUICDMSlideHandle inSourceSlide, CUICDMSlideHandle inDestSlide,
                           TInstanceHandleList &outInstances, TSlideEntryList &outReferencedEntries)
{
    TPropertyHandlePropertyInfoPairList::const_iterator theFind =
        find_if<TPropertyHandlePropertyInfoPairList::const_iterator>(
            inInfoPairList, std::bind(PropertyHandlePairEquals,
                                      std::placeholders::_1, get<1>(inEntry)));
    if (theFind != inInfoPairList.end()) {
        TPropertyInstanceInfoPtr theInfo(theFind->second);
        Qt3DSDMInstanceHandle theReferenced(theInfo->GetInstanceForProperty(get<2>(inEntry)));
        if (theReferenced.Valid()
            && !exists(outInstances, std::bind(equal_to<int>(), theReferenced,
                                               std::placeholders::_1))) {
            insert_unique(outInstances, theReferenced);
            SValue theNewValue(
                theInfo->CreateInstanceForProperty(inSourceSlide, inDestSlide, theReferenced));
            outReferencedEntries.push_back(
                make_tuple(get<0>(inEntry), get<1>(inEntry), theNewValue));
        }
    }
}

void CopySpecificAnimation(CUICDMSlideHandle inMaster, CUICDMSlideHandle inTarget,
                           TInstancePropertyPair inPropertyPair, TAnimationCorePtr inAnimationCore,
                           size_t inIndex)
{
    CUICDMAnimationHandle theAnimation = inAnimationCore->GetAnimation(
        inMaster, inPropertyPair.first, inPropertyPair.second, inIndex);
    if (theAnimation.Valid())
        CopyAnimation(inAnimationCore, theAnimation, inTarget, inPropertyPair.first,
                      inPropertyPair.second, inIndex);
}

void CopyAnimationIfExist(CUICDMSlideHandle inMaster, CUICDMSlideHandle inTarget,
                          TInstancePropertyPair inPropertyPair, TPropertySystemPtr inPropertySystem,
                          TAnimationCorePtr inAnimationCore)
{
    DataModelDataType::Value thePropertyType = inPropertySystem->GetDataType(inPropertyPair.second);
    std::tuple<bool, size_t> theArity = GetDatatypeAnimatableAndArity(thePropertyType);
    if (std::get<0>(theArity))
        do_times(std::get<1>(theArity), std::bind(CopySpecificAnimation, inMaster, inTarget,
                                               inPropertyPair, inAnimationCore,
                                                  std::placeholders::_1));
}

void SetEntryValueIfNotReferenced(const TSlideEntry &inEntry,
                                  const TInstanceHandleList &inReferencedInstances,
                                  const TSlideEntryList &inReferencedEntries,
                                  CUICDMSlideHandle inSource, CUICDMSlideHandle inDestSlide,
                                  TPropertySystemPtr inPropertySystem, TSlideCorePtr inDestCore,
                                  TAnimationCorePtr inAnimationCore)
{
    // Don't copy referenced instance properties.
    if (exists(inReferencedInstances, std::bind(equal_to<int>(), get<0>(inEntry),
                                                std::placeholders::_1)))
        return;
    TSlideEntryList::const_iterator theFind = find_if<TSlideEntryList::const_iterator>(
        inReferencedEntries,
        std::bind(CSimpleSlideCore::PropertyFound, get<0>(inEntry), get<1>(inEntry),
                  std::placeholders::_1));
    if (theFind != inReferencedEntries.end())
        inDestCore->ForceSetInstancePropertyValue(inDestSlide, get<0>(inEntry), get<1>(inEntry),
                                                  get<2>(*theFind));
    else
        inDestCore->ForceSetInstancePropertyValue(inDestSlide, get<0>(inEntry), get<1>(inEntry),
                                                  get<2>(inEntry));
    CopyAnimationIfExist(inSource, inDestSlide, make_pair(get<0>(inEntry), get<1>(inEntry)),
                         inPropertySystem, inAnimationCore);
}

CUICDMSlideHandle SSlideSystem::CreateSlide(CUICDMSlideHandle inMaster, int inIndex)
{
    return DuplicateSlide(inMaster, inIndex);
}

CUICDMSlideHandle SSlideSystem::DuplicateSlide(CUICDMSlideHandle inSourceSlide, int inIndex)
{
    CUICDMSlideHandle theMaster = GetMasterSlide(inSourceSlide);
    Qt3DSDMInstanceHandle theSlideInstance = m_DataCore->CreateInstance();
    m_DataCore->DeriveInstance(theSlideInstance, m_SlideInstance);
    CUICDMSlideHandle retval = m_SlideCore->CreateSlide(theSlideInstance);

    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(theMaster, theChildren);
    m_SlideCore->DeriveSlide(retval, theMaster, inIndex - 1);
    int finalIndex = m_SlideCore->GetChildIndex(theMaster, retval) + 1;
    if (!theChildren.empty()) {
        // If the master was passed in, we get the intersecting properties
        // of the master and first child.
        // if another slide was passed in, we again do the intersection but we
        // take the values from the source slide instead of the master.
        TSlideEntryList theIntersectingEntries;
        if (inSourceSlide == theMaster)
            m_SlideCore->GetIntersectingProperties(theMaster, theChildren.at(0),
                                                   theIntersectingEntries);
        else
            m_SlideCore->GetIntersectingProperties(inSourceSlide, theMaster,
                                                   theIntersectingEntries);

        // duplicates the instance properties, from source slide (could be master (a new slide
        // command) or another other slides (duplicate slide) to the newly created slide
        m_DataCore->CopyInstanceProperties(this->GetSlideInstance(inSourceSlide), theSlideInstance);

        TInstanceHandleList theReferencedInstances;
        TSlideEntryList theReferencedEntries;
        do_all(theIntersectingEntries,
               std::bind(AddReferencedInstance, std::placeholders::_1,
                         std::cref(m_PropertyInfoPairList),
                         inSourceSlide, retval, std::ref(theReferencedInstances),
                         std::ref(theReferencedEntries)));
        do_all(theIntersectingEntries,
               std::bind(SetEntryValueIfNotReferenced, std::placeholders::_1,
                         std::cref(theReferencedInstances),
                         std::cref(theReferencedEntries), inSourceSlide, retval,
                         m_PropertySystem, m_SlideCore, m_AnimationCore));
    }
    GetSignalSender()->SendSlideCreated(theMaster, finalIndex, retval);
    return retval;
}

CUICDMSlideHandle SSlideSystem::GetMasterSlide(CUICDMSlideHandle inSlide) const
{
    if (inSlide.Valid() && m_SlideCore->IsSlide(inSlide)) {
        CUICDMSlideHandle theParent = m_SlideCore->GetParentSlide(inSlide);
        if (theParent.Valid())
            return theParent;
        return inSlide;
    }
    return 0;
}

bool SSlideSystem::IsMasterSlide(CUICDMSlideHandle inSlide) const
{
    CUICDMSlideHandle theParent = m_SlideCore->GetParentSlide(inSlide);
    if (!theParent.Valid())
        return true;
    else
        return false;
}

inline bool GraphGuidMatches(CUICDMSlideGraphHandle inGraph, TSlideGraphCorePtr inSlideGraph,
                             TSlideCorePtr inSlideCore, TDataCorePtr inDataCore,
                             Qt3DSDMPropertyHandle inProperty, SValue inValue)
{
    CUICDMSlideHandle theMaster = inSlideGraph->GetGraphRoot(inGraph);
    Qt3DSDMInstanceHandle theInstance = inSlideCore->GetSlideInstance(theMaster);
    SValue theValue;
    if (inDataCore->GetInstancePropertyValue(theInstance, inProperty, theValue)
        && Equals(inValue, theValue.toOldSkool()))
        return true;
    return false;
}

CUICDMSlideHandle SSlideSystem::GetMasterSlideByComponentGuid(SLong4 inGuid) const
{
    TSlideGraphHandleList theGraphs;
    m_SlideGraphCore->GetSlideGraphs(theGraphs);
    TSlideGraphHandleList::iterator theFind = find_if<TSlideGraphHandleList::iterator>(
        theGraphs, std::bind(GraphGuidMatches,
                             std::placeholders::_1, m_SlideGraphCore, m_SlideCore, m_DataCore,
                             m_ComponentGuid, inGuid));
    if (theFind != theGraphs.end())
        return m_SlideGraphCore->GetGraphRoot(*theFind);
    return 0;
}

void InsertIfReferencedProperty(const TSlideEntry &inEntry,
                                const TPropertyHandlePropertyInfoPairList &inRefProperties,
                                TInstanceHandleList &inInstances)
{
    TPropertyHandlePropertyInfoPairList::const_iterator theFind =
        find_if<TPropertyHandlePropertyInfoPairList::const_iterator>(
            inRefProperties, std::bind(PropertyHandlePairEquals, std::placeholders::_1, get<1>(inEntry)));
    if (theFind != inRefProperties.end()) {
        Qt3DSDMInstanceHandle theInstance(theFind->second->GetInstanceForProperty(get<2>(inEntry)));
        if (theInstance.Valid())
            inInstances.push_back(theInstance);
    }
}

void SSlideSystem::InsertEntryAndPropertyInstance(const TSlideEntry &inEntry,
                                                  TInstanceHandleList &inInstances,
                                                  CUICDMSlideHandle inSlide)
{
    Qt3DSDMInstanceHandle theEntryInstance = get<0>(inEntry);
    if (find(inInstances.begin(), inInstances.end(), theEntryInstance) == inInstances.end()) {
        TGraphSlidePair thePair = m_SlideGraphCore->GetAssociatedGraph(theEntryInstance);
        if (thePair.second == inSlide) // if this instance belongs to this slide
        {
            // get all references belong to this instance (ex: image instances that belong to this
            // material instance)
            qt3dsdm::SValue theValue;
            for (TPropertyHandlePropertyInfoPairList::iterator theIter =
                     m_PropertyInfoPairList.begin();
                 theIter != m_PropertyInfoPairList.end(); ++theIter) {
                if (m_DataCore->HasAggregateInstanceProperty(
                        theEntryInstance,
                        theIter->first) // check if the property exists before querying the value
                    && m_DataCore->GetInstancePropertyValue(
                           theEntryInstance, theIter->first,
                           theValue)) // this function may throw error if the property doesn't exist
                {
                    Qt3DSDMInstanceHandle theInstance(
                        theIter->second->GetInstanceForProperty(theValue.toOldSkool()));
                    if (theInstance.Valid())
                        inInstances.push_back(theInstance);
                }
            }
            // get this instance as well
            inInstances.push_back(theEntryInstance);
        }
    }
}

// Delete the referenced instances of this slide
// This function is very similar to GetReferencedInstances
// You change one function, you need to change the other function
void SSlideSystem::DeleteReferencedInstances(CUICDMSlideHandle inSlide)
{
    // Recursively delete the children of this slide
    // Usually the slide has children if it is a Master slide (for example when deleting a
    // Component)
    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(inSlide, theChildren);
    do_all(theChildren, std::bind(&SSlideSystem::DeleteReferencedInstances, this,
                                  std::placeholders::_1));

    // Delete the referenced instances from this slide
    TSlideEntryList theEntries;
    m_SlideCore->GetSlidePropertyEntries(inSlide, theEntries);

    // Run through all entries, if you find a reference property delete the associated instance.
    // This is for properties that are set on this slide (because you can set property on slide or
    // on instance)
    TInstanceHandleList theReferencedInstances;
    do_all(theEntries,
           std::bind(InsertIfReferencedProperty, std::placeholders::_1,
                     std::cref(m_PropertyInfoPairList),
                     std::ref(theReferencedInstances)));
    do_all(theReferencedInstances, std::bind(&IDataCore::DeleteInstance, m_DataCore,
                                             std::placeholders::_1));

    // Run through all entries, delete all instances that belong to this slide and its reference
    // property instances
    // This is for properties that are set on instance
    theReferencedInstances.clear();
    do_all(theEntries, std::bind(&SSlideSystem::InsertEntryAndPropertyInstance, this,
                                 std::placeholders::_1,
                                 std::ref(theReferencedInstances), inSlide));
    do_all(theReferencedInstances, std::bind(&IDataCore::DeleteInstance, m_DataCore,
                                             std::placeholders::_1));
}

// Get the referenced instances of this slide
// This function is very similar to DeleteReferencedInstances
// You change one function, you need to change the other function
void SSlideSystem::GetReferencedInstances(CUICDMSlideHandle inSlide,
                                          TInstanceHandleList &outReferencedInstances)
{
    // Recursively get the children of this slide
    // Usually the slide has children if it is a Master slide (for example Component)
    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(inSlide, theChildren);
    do_all(theChildren, std::bind(&SSlideSystem::GetReferencedInstances, this,
                                  std::placeholders::_1,
                                  std::ref(outReferencedInstances)));

    // Get the referenced instances from this slide
    TSlideEntryList theEntries;
    m_SlideCore->GetSlidePropertyEntries(inSlide, theEntries);

    // Run through all entries, if you find a reference property get the associated instance.
    // This is for properties that are set on this slide (because you can set property on slide or
    // on instance)
    do_all(theEntries,
           std::bind(InsertIfReferencedProperty, std::placeholders::_1,
                     std::cref(m_PropertyInfoPairList),
                     std::ref(outReferencedInstances)));

    // Run through all entries, get all instances that belong to this slide and its reference
    // property instances
    // This is for properties that are set on instance
    do_all(theEntries, std::bind(&SSlideSystem::InsertEntryAndPropertyInstance, this,
                                 std::placeholders::_1,
                                 std::ref(outReferencedInstances), inSlide));
}

void SSlideSystem::DeleteSlideByIndex(CUICDMSlideHandle inMaster, size_t inIndex)
{
    CUICDMSlideHandle theChild = GetSlideByIndex(inMaster, inIndex);
    TInstanceHandleList theInstances;
    if (theChild.Valid()) {
        DeleteReferencedInstances(theChild);
        m_SlideCore->DeleteSlide(theChild, theInstances);
        do_all(theInstances, std::bind(&IDataCore::DeleteInstance, m_DataCore,
                                       std::placeholders::_1));
    }
    if (inIndex == 0)
        GetSignalSender()->SendMasterDeleted(inMaster);
    else
        GetSignalSender()->SendSlideDeleted(inMaster, (int)inIndex, theChild);
}

void SSlideSystem::GetSlideReferencedInstances(CUICDMSlideHandle inMaster, size_t inIndex,
                                               TInstanceHandleList &outReferencedInstances)
{
    outReferencedInstances.clear();
    CUICDMSlideHandle theChild = GetSlideByIndex(inMaster, inIndex);
    if (theChild.Valid()) {
        GetReferencedInstances(theChild, outReferencedInstances);
    }
}

CUICDMSlideHandle SSlideSystem::GetSlideByIndex(CUICDMSlideHandle inMaster, size_t inIndex) const
{
    if (inIndex == 0)
        return inMaster;
    --inIndex;
    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(inMaster, theChildren);
    if (inIndex < theChildren.size())
        return theChildren[inIndex];
    return CUICDMSlideHandle();
}

void SSlideSystem::SetActiveSlide(CUICDMSlideHandle inMaster, size_t inIndex)
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetSlideGraph(inMaster);
    CUICDMSlideHandle theActiveSlide = inMaster;
    if (inIndex > 0)
        theActiveSlide = GetSlideByIndex(inMaster, inIndex);
    CUICDMSlideHandle theOldSlide = m_SlideGraphCore->GetGraphActiveSlide(theGraph);
    m_SlideGraphCore->SetGraphActiveSlide(theGraph, theActiveSlide);
    GetSignalSender()->SendActiveSlide(inMaster, (int)inIndex, theOldSlide, theActiveSlide);
}

size_t SSlideSystem::GetSlideCount(CUICDMSlideHandle inMaster) const
{
    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(inMaster, theChildren);
    return 1 + theChildren.size();
}

void SSlideSystem::RearrangeSlide(CUICDMSlideHandle inMaster, size_t inOldIndex, size_t inNewIndex)
{
    if (inOldIndex == 0)
        throw RearrangeSlideArgumentsMustNotBeZero(L"");
    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(inMaster, theChildren);
    CUICDMSlideHandle theChild = theChildren.at(inOldIndex - 1);
    m_SlideCore->DeriveSlide(theChild, inMaster, (int)inNewIndex - 1);
    GetSignalSender()->SendSlideRearranged(inMaster, (int)inOldIndex, (int)inNewIndex);
}

void SSlideSystem::SetComponentSeconds(CUICDMSlideHandle inSlide, float inSeconds)
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetSlideGraph(GetMasterSlide(inSlide));
    m_SlideCore->SetSlideTime(m_SlideGraphCore->GetGraphActiveSlide(theGraph), inSeconds);
}

float SSlideSystem::GetComponentSeconds(CUICDMSlideHandle inSlide) const
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetSlideGraph(GetMasterSlide(inSlide));
    return m_SlideCore->GetSlideTime(m_SlideGraphCore->GetGraphActiveSlide(theGraph));
}

long SSlideSystem::GetComponentSecondsLong(CUICDMSlideHandle inSlide) const
{
    float seconds(GetComponentSeconds(inSlide));
    return static_cast<long>((seconds * 1000) + .5f);
}

long SSlideSystem::GetComponentSecondsLong(Qt3DSDMInstanceHandle inInstance) const
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetAssociatedGraph(inInstance).first;
    float seconds = m_SlideCore->GetSlideTime(m_SlideGraphCore->GetGraphActiveSlide(theGraph));
    return static_cast<long>((seconds * 1000) + .5f);
}

SInstanceSlideInformation
SSlideSystem::GetInstanceSlideInformation(Qt3DSDMInstanceHandle inInstance) const
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
    CUICDMSlideHandle theAssociatedSlide(theGraphSlidePair.second);
    if (theAssociatedSlide.Valid() == false)
        return SInstanceSlideInformation();
    CUICDMSlideHandle theMasterSlide(GetMasterSlide(theGraphSlidePair.second));
    CUICDMSlideHandle theActiveSlide(
        m_SlideGraphCore->GetGraphActiveSlide(theGraphSlidePair.first));
    float seconds = m_SlideCore->GetSlideTime(theActiveSlide);
    long theMilliseconds = static_cast<long>((seconds * 1000) + .5f);
    return SInstanceSlideInformation(theAssociatedSlide, theMasterSlide, theActiveSlide,
                                     theMilliseconds);
}

/**
 * Use the instance for storing information such as name, or the GUID of the object
 * this slide links to.
 */
Qt3DSDMInstanceHandle SSlideSystem::GetSlideInstance(CUICDMSlideHandle inSlide) const
{
    return m_SlideCore->GetSlideInstance(inSlide);
}
/**
 *	Reverse lookup into the slide system so you can match slides to instances.
 */
CUICDMSlideHandle SSlideSystem::GetSlideByInstance(Qt3DSDMInstanceHandle inSlide) const
{
    return m_SlideCore->GetSlideByInstance(inSlide);
}

/**
 *	Slide may be either a master slide
 */
void SSlideSystem::AssociateInstanceWithSlide(CUICDMSlideHandle inSlide,
                                              Qt3DSDMInstanceHandle inInstance)
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetSlideGraph(GetMasterSlide(inSlide));
    m_SlideGraphCore->AssociateInstance(theGraph, inSlide, inInstance);
    GetSignalSender()->SendInstanceAssociated(inSlide, inInstance);
}

CUICDMSlideHandle SSlideSystem::GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance) const
{
    return m_SlideGraphCore->GetAssociatedGraph(inInstance).second;
}

/**
 *	This gets the instances that resides int the SlideGraph, i.e. all the instances in all the
 *slides for this component/scene
 *	TODO rename this to make it more clear
 */
void SSlideSystem::GetAssociatedInstances(CUICDMSlideHandle inMaster,
                                          TSlideInstancePairList &outAssociations) const
{
    m_SlideGraphCore->GetAssociatedInstances(
        m_SlideGraphCore->GetSlideGraph(GetMasterSlide(inMaster)), outAssociations);
}

/**
 *	Gets all the instances in this slide
 */
void SSlideSystem::GetAssociatedInstances(CUICDMSlideHandle inSlide,
                                          TInstanceHandleList &outAssociations) const
{
    CUICDMSlideHandle theMasterSlide = GetMasterSlide(inSlide);
    TSlideInstancePairList theGraphInstances;
    m_SlideGraphCore->GetAssociatedInstances(m_SlideGraphCore->GetSlideGraph(theMasterSlide),
                                             theGraphInstances);
    for (TSlideInstancePairList::const_iterator theIter = theGraphInstances.begin();
         theIter != theGraphInstances.end(); ++theIter) {
        if (theIter->first == inSlide || theIter->first == theMasterSlide) {
            // in the current slide or master slide
            outAssociations.push_back(theIter->second);
        }
    }
}

void DeleteInstanceIfExistsAsProperty(CUICDMSlideHandle inSlide, Qt3DSDMInstanceHandle inInstance,
                                      Qt3DSDMPropertyHandle inProperty, TSlideCorePtr inSlideCore,
                                      TPropertyInstanceInfoPtr inPropertyInfoPtr,
                                      TDataCorePtr inDataCore)
{
    SValue theValue;
    if (inSlideCore->GetSpecificInstancePropertyValue(inSlide, inInstance, inProperty, theValue)) {
        Qt3DSDMInstanceHandle theInstance(inPropertyInfoPtr->GetInstanceForProperty(theValue));
        if (theInstance.Valid())
            inDataCore->DeleteInstance(theInstance);
    }
}

void SSlideSystem::LinkProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty)
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetAssociatedGraph(inInstance).first;
    CUICDMSlideHandle theSlide = m_SlideGraphCore->GetGraphRoot(theGraph);
    TPropertyHandlePropertyInfoPairList::const_iterator theFind =
        find_if<TPropertyHandlePropertyInfoPairList::const_iterator>(
            m_PropertyInfoPairList, std::bind(PropertyHandlePairEquals,
                                              std::placeholders::_1, inProperty));
    if (theFind != m_PropertyInfoPairList.end()) {
        TSlideHandleList theChildren;
        m_SlideCore->GetChildSlides(theSlide, theChildren);
        do_all(theChildren, std::bind(DeleteInstanceIfExistsAsProperty, std::placeholders::_1, inInstance,
                                        inProperty, m_SlideCore, theFind->second, m_DataCore));
    }
    m_SlideCore->ClearChildrenPropertyValues(theSlide, inInstance, inProperty);
    GetSignalSender()->SendPropertyLinked(theSlide, inInstance, inProperty);
}

void ClearPropertyValueIfLinked(CUICDMSlideHandle inMaster, const TSlideHandleList &inChildren,
                                Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                TSlideCorePtr inSlideCore)
{
    if (inChildren.empty())
        return;

    if (inSlideCore->ContainsProperty(inChildren.at(0), inInstance,
                                      inProperty)) // if property is linked,
        inSlideCore->ClearChildrenPropertyValues(inMaster, inInstance,
                                                 inProperty); // get it off non-master slides
}

void SetReferencedEntryValue(CUICDMSlideHandle inMaster, CUICDMSlideHandle inDestSlide,
                             TPropertyInstanceInfoPtr inInfo, Qt3DSDMInstanceHandle inInstance,
                             Qt3DSDMPropertyHandle inProperty,
                             Qt3DSDMInstanceHandle inReferencedInstance, TSlideCorePtr inSlideCore)
{
    SValue theNewValue(
        inInfo->CreateInstanceForProperty(inMaster, inDestSlide, inReferencedInstance));
    inSlideCore->ForceSetInstancePropertyValue(inDestSlide, inInstance, inProperty, theNewValue);
}

void SSlideSystem::UnlinkProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty)
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetAssociatedGraph(inInstance).first;
    CUICDMSlideHandle theSlide = m_SlideGraphCore->GetGraphRoot(theGraph);
    SValue theValue;
    SValue theTempValue;
    if (!m_SlideCore->GetInstancePropertyValue(theSlide, inInstance, inProperty, theValue)) {
        if (!m_PropertySystem->GetInstancePropertyValue(inInstance, inProperty, theTempValue))
            throw PropertyLinkError(L"");
        theValue = theTempValue.toOldSkool();
    }

    m_SlideCore->ForceSetInstancePropertyValue(theSlide, inInstance, inProperty, theValue);
    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(theSlide, theChildren);

    Qt3DSDMInstanceHandle theReferenced;
    TPropertyHandlePropertyInfoPairList::const_iterator theFind =
        find_if<TPropertyHandlePropertyInfoPairList::const_iterator>(
            m_PropertyInfoPairList, std::bind(PropertyHandlePairEquals,
                                              std::placeholders::_1, inProperty));
    TPropertyInstanceInfoPtr theInfo;
    if (theFind != m_PropertyInfoPairList.end()) {
        theInfo = theFind->second;
        theReferenced = theInfo->GetInstanceForProperty(theValue);
    }
    if (theReferenced.Valid()) {
        TPropertyHandleList theProperties;
        m_PropertySystem->GetAggregateInstanceProperties(
            theReferenced,
            theProperties); // TODO: We should make the method return the custom properties.
        // Remove the property instance's unlinked properties from non-master slides.
        do_all(theProperties,
               std::bind(ClearPropertyValueIfLinked, theSlide, std::cref(theChildren),
                           theReferenced, std::placeholders::_1, m_SlideCore));
        // Remove all property values from the children from that instance.
        do_all(theChildren, std::bind(SetReferencedEntryValue, theSlide,
                                      std::placeholders::_1, theInfo, inInstance,
                                      inProperty, theReferenced, m_SlideCore));
    } else {
        do_all(theChildren, std::bind(&ISlideCore::ForceSetInstancePropertyValue, m_SlideCore,
                                      std::placeholders::_1,
                                      inInstance, inProperty, theValue));
        do_all(theChildren,
               std::bind(CopyAnimationIfExist, theSlide, std::placeholders::_1, make_pair(inInstance, inProperty),
                           m_PropertySystem, m_AnimationCore));
    }
    GetSignalSender()->SendPropertyUnlinked(theSlide, inInstance, inProperty);
}

bool SSlideSystem::IsPropertyLinked(Qt3DSDMInstanceHandle inInstance,
                                    Qt3DSDMPropertyHandle inProperty) const
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
    CUICDMSlideGraphHandle theGraph = theGraphSlidePair.first;
    if (!theGraph.Valid())
        return false;

    CUICDMSlideHandle theSlide = m_SlideGraphCore->GetGraphRoot(theGraph);
    if (theGraphSlidePair.second != theSlide)
        return false;

    TSlideHandleList theChildren;
    m_SlideCore->GetChildSlides(theSlide, theChildren);
    bool containsProperty = false;
    for (TSlideHandleList::iterator theIter = theChildren.begin();
         theIter != theChildren.end() && !containsProperty; ++theIter) {
        containsProperty =
            containsProperty || m_SlideCore->ContainsProperty(*theIter, inInstance, inProperty);
    }

    return !containsProperty;
}

bool SSlideSystem::CanPropertyBeLinked(Qt3DSDMInstanceHandle inInstance,
                                       Qt3DSDMPropertyHandle inProperty) const
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
    if (theGraphSlidePair.first.Valid()
        && theGraphSlidePair.second == m_SlideGraphCore->GetGraphRoot(theGraphSlidePair.first))
        return true;
    return false;
}

bool SSlideSystem::GetSlidePropertyValue(size_t inSlide, Qt3DSDMInstanceHandle inInstance,
                                         Qt3DSDMPropertyHandle inProperty, SValue &outValue)
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
    CUICDMSlideGraphHandle theGraph = theGraphSlidePair.first;
    if (!theGraph.Valid())
        return false;
    CUICDMSlideHandle theSlide = GetSlideByIndex(m_SlideGraphCore->GetGraphRoot(theGraph), inSlide);
    if (!theSlide.Valid())
        return false;
    return m_SlideCore->GetSpecificInstancePropertyValue(theSlide, inInstance, inProperty,
                                                         outValue);
}

void AddEntriesToHash(const TSlideEntryList &theSlideEntries, TSlideEntryHash &theEntryHash,
                      TInstancePropertyPairList &outProperties)
{
    for (size_t idx = 0, end = theSlideEntries.size(); idx < end; ++idx) {
        const TSlideEntry &theEntry(theSlideEntries[idx]);
        pair<TSlideEntryHash::iterator, bool> insertRecord(theEntryHash.insert(
            make_pair(TSlideInstancePropertyPair(get<0>(theEntry), get<1>(theEntry)),
                      SInternValue::ISwearThisHasAlreadyBeenInternalized(get<2>(theEntry)))));
        if (insertRecord.second)
            outProperties.push_back(insertRecord.first->first);
    }
}

void SSlideSystem::GetUnionOfProperties(CUICDMSlideHandle inSlide1, CUICDMSlideHandle inSlide2,
                                        TInstancePropertyPairList &outProperties) const
{
    TSlideEntryHash theEntryHash;
    TSlideEntryList theSlideEntries;
    if (m_SlideCore->IsSlide(inSlide1)) {
        m_SlideCore->GetSlidePropertyEntries(inSlide1, theSlideEntries);
        AddEntriesToHash(theSlideEntries, theEntryHash, outProperties);
    }
    if (m_SlideCore->IsSlide(inSlide2)) {
        m_SlideCore->GetSlidePropertyEntries(inSlide2, theSlideEntries);
        AddEntriesToHash(theSlideEntries, theEntryHash, outProperties);
    }
}

void SSlideSystem::SetActiveSlide(CUICDMSlideHandle inSlide)
{
    CUICDMSlideHandle theMaster = GetMasterSlide(inSlide);
    int theIndex = GetSlideIndex(inSlide);
    SetActiveSlide(theMaster, theIndex);
}

CUICDMSlideHandle SSlideSystem::GetAssociatedSlide(Qt3DSDMInstanceHandle inInstance,
                                                   Qt3DSDMPropertyHandle inProperty) const
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
    CUICDMSlideGraphHandle theGraph = theGraphSlidePair.first;
    if (!theGraph.Valid())
        return 0;
    CUICDMSlideHandle theSlide = m_SlideGraphCore->GetGraphActiveSlide(theGraph);
    if (theSlide.Valid()) {
        if (m_SlideCore->ContainsProperty(theSlide, inInstance, inProperty))
            return theSlide;
        theSlide = m_SlideCore->GetParentSlide(theSlide);
        if (theSlide.Valid() && m_SlideCore->ContainsProperty(theSlide, inInstance, inProperty))
            return theSlide;
        return theGraphSlidePair.second;
    }
    return 0;
}

bool SSlideSystem::SlideValid(CUICDMSlideHandle inSlide) const
{
    return m_SlideCore->HandleValid(inSlide);
}

int SSlideSystem::GetSlideIndex(CUICDMSlideHandle inSlide) const
{
    CUICDMSlideHandle theMaster = GetMasterSlide(inSlide);
    if (inSlide.Valid() && inSlide != theMaster)
        return m_SlideCore->GetChildIndex(theMaster, inSlide) + 1;
    return 0;
}

int SSlideSystem::GetActiveSlideIndex(CUICDMSlideHandle inMaster) const
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetSlideGraph(inMaster);
    CUICDMSlideHandle theActiveSlide = m_SlideGraphCore->GetGraphActiveSlide(theGraph);
    if (theActiveSlide == inMaster)
        return 0;
    return GetSlideIndex(theActiveSlide);
}

CUICDMSlideHandle SSlideSystem::GetActiveSlide(CUICDMSlideHandle inMaster) const
{
    CUICDMSlideGraphHandle theGraph = m_SlideGraphCore->GetSlideGraph(inMaster);
    return m_SlideGraphCore->GetGraphActiveSlide(theGraph);
}

Qt3DSDMInstanceHandle SSlideSystem::GetSlideSelectedInstance(CUICDMSlideHandle inSlide) const
{
    TIntIntMap::const_iterator theIter = m_SlideSelectedInstances.find(inSlide);
    if (theIter != m_SlideSelectedInstances.end() && m_DataCore->IsInstance(theIter->second))
        return theIter->second;
    return 0;
}

void SSlideSystem::SetSlideSelectedInstance(CUICDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance)
{
    m_SlideSelectedInstances[inSlide] = inInstance;
}

void SSlideSystem::RegisterPropertyInstance(Qt3DSDMPropertyHandle inPropertyHandle,
                                            TPropertyInstanceInfoPtr inPropertyInfo)
{
    m_PropertyInfoPairList.push_back(make_pair(inPropertyHandle, inPropertyInfo));
}

ISlideSystemSignalProvider *SSlideSystem::GetSignalProvider()
{
    return dynamic_cast<ISlideSystemSignalProvider *>(m_Signaller.get());
}

ISlideSystemSignalSender *SSlideSystem::GetSignalSender()
{
    return dynamic_cast<ISlideSystemSignalSender *>(m_Signaller.get());
}

qt3dsdm::CUICDMSlideHandle SSlideSystem::GetApplicableSlide(Qt3DSDMInstanceHandle inHandle,
                                                          Qt3DSDMPropertyHandle inProperty)
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inHandle);
    if (!theGraphSlidePair.first.Valid())
        return CUICDMSlideHandle();

    CUICDMSlideHandle theMaster = m_SlideGraphCore->GetGraphRoot(theGraphSlidePair.first);
    if (theGraphSlidePair.second != theMaster)
        return theGraphSlidePair.second;

    CUICDMSlideHandle theActive = m_SlideGraphCore->GetGraphActiveSlide(theGraphSlidePair.first);
    if (m_SlideCore->ContainsProperty(theActive, inHandle, inProperty))
        return theActive;

    return theMaster;
}

bool SSlideSystem::GetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                            Qt3DSDMInstanceHandle inInstance,
                                            Qt3DSDMPropertyHandle inProperty, SValue &outValue) const
{
    return m_SlideCore->GetInstancePropertyValue(inSlide, inInstance, inProperty, outValue);
}

bool SSlideSystem::GetCanonicalInstancePropertyValue(Qt3DSDMInstanceHandle inInstance,
                                                     Qt3DSDMPropertyHandle inProperty,
                                                     SValue &outValue) const
{
    TGraphSlidePair theGraphSlidePair = m_SlideGraphCore->GetAssociatedGraph(inInstance);
    if (theGraphSlidePair.first.Valid()) {
        // Check to see if the object is on the master slide;
        if (theGraphSlidePair.second == m_SlideGraphCore->GetGraphRoot(theGraphSlidePair.first)) {
            TSlideHandleList theChildren;
            m_SlideCore->GetChildSlides(theGraphSlidePair.second, theChildren);
            // See if the value exists on slide 1.
            if (!theChildren.empty()
                && m_SlideCore->GetSpecificInstancePropertyValue(theChildren.at(0), inInstance,
                                                                 inProperty, outValue))
                return true;
        }

        if (m_SlideCore->GetSpecificInstancePropertyValue(theGraphSlidePair.second, inInstance,
                                                          inProperty, outValue))
            return true;
    }
    return false;
}

void SSlideSystem::ForceSetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                                 Qt3DSDMInstanceHandle inInstance,
                                                 Qt3DSDMPropertyHandle inProperty,
                                                 const SValue &inValue)
{
    m_SlideCore->ForceSetInstancePropertyValue(inSlide, inInstance, inProperty, inValue);
}
}
