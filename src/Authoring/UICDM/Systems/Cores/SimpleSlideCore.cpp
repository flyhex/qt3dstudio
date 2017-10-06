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
#include "UICDMPrefix.h"
#include "SimpleSlideCore.h"

using namespace std;
#ifdef _WIN32
#pragma warning(disable : 4503)
#endif

namespace UICDM {

CUICDMSlideHandle CSimpleSlideCore::CreateSlide(CUICDMInstanceHandle inInstance)
{
    int nextId = GetNextId();
    return CreateSlideWithHandle(nextId, inInstance);
}

CUICDMInstanceHandle CSimpleSlideCore::GetSlideInstance(CUICDMSlideHandle inSlide) const
{
    return GetSlideNF(inSlide, m_Objects)->m_Instance;
}

inline bool SlideInstanceMatches(const THandleObjectPair &inPair, int inInstance)
{
    if (inPair.second->GetType() == CHandleObject::EHandleObjectTypeSSlide
        && static_cast<const SSlide *>(inPair.second.get())->m_Instance == inInstance)
        return true;
    return false;
}

CUICDMSlideHandle CSimpleSlideCore::GetSlideByInstance(CUICDMInstanceHandle inInstance) const
{
    THandleObjectMap::const_iterator theSlide =
        find_if(m_Objects.begin(), m_Objects.end(),
                std::bind(SlideInstanceMatches,
                          std::placeholders::_1, inInstance.GetHandleValue()));
    if (theSlide != m_Objects.end())
        return theSlide->first;
    throw SlideNotFound(L"");
}

void RecurseDeleteSlide(CUICDMSlideHandle inSlide, THandleObjectMap &inObjects,
                        TInstanceHandleList &outInstances)
{
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, inObjects);
    do_all(theSlide->m_Children,
           std::bind(RecurseDeleteSlide, std::placeholders::_1,
                     std::ref(inObjects), std::ref(outInstances)));
    outInstances.push_back(theSlide->m_Instance);
    CHandleBase::EraseHandle(inSlide, inObjects);
}

void CSimpleSlideCore::DeleteSlide(CUICDMSlideHandle inSlide, TInstanceHandleList &outInstances)
{
    SSlide *theSlide = GetSlideNF(inSlide, m_Objects);
    if (theSlide->m_Parent) {
        SSlide *theParent = GetSlideNF(theSlide->m_Parent, m_Objects);
        erase_if(theParent->m_Children, std::bind(equal_to<int>(), theSlide->m_Handle,
                                                  std::placeholders::_1));
    }
    RecurseDeleteSlide(inSlide, m_Objects, outInstances);
}

template <typename TDataType, typename TVectorType>
inline void MaybeAddObject(const THandleObjectPair &inPair, vector<TVectorType> &outVectorItems)
{
    if (inPair.second->GetType() == TDataType::s_Type)
        outVectorItems.push_back(inPair.first);
}

void CSimpleSlideCore::GetSlides(TSlideHandleList &outSlides) const
{
    do_all(m_Objects,
           std::bind(MaybeAddObject<SSlide, CUICDMSlideHandle>,
                     std::placeholders::_1, std::ref(outSlides)));
}

float CSimpleSlideCore::GetSlideTime(CUICDMSlideHandle inSlide) const
{
    return GetSlideNF(inSlide, m_Objects)->m_Time;
}

void CSimpleSlideCore::SetSlideTime(CUICDMSlideHandle inSlide, float inNewTime)
{
    GetSlideNF(inSlide, m_Objects)->m_Time = inNewTime;
}

void CSimpleSlideCore::DeriveSlide(CUICDMSlideHandle inSlide, CUICDMSlideHandle inParent,
                                   int inIndex)
{
    int oldParent = GetSlideNF(inSlide, m_Objects)->m_Parent;
    if (oldParent)
        erase_if(GetSlideNF(oldParent, m_Objects)->m_Children,
                 std::bind(equal_to<int>(), inSlide, std::placeholders::_1));
    if (inParent.Valid()) {
        SSlide *theParent = GetSlideNF(inParent, m_Objects);
        if (exists(theParent->m_Children, std::bind(equal_to<int>(), inSlide,
                                                    std::placeholders::_1)))
            throw SlideDerivationError(L"Already derived");
        if (inIndex < 0 || inIndex >= (int)theParent->m_Children.size())
            inIndex = (int)theParent->m_Children.size();
        theParent->m_Children.insert(theParent->m_Children.begin() + inIndex,
                                     inSlide.GetHandleValue());
    }
    GetSlideNF(inSlide, m_Objects)->m_Parent = inParent;
}

CUICDMSlideHandle CSimpleSlideCore::GetParentSlide(CUICDMSlideHandle inSlide) const
{
    return GetSlideNF(inSlide, m_Objects)->m_Parent;
}

void CSimpleSlideCore::GetChildSlides(CUICDMSlideHandle inSlide,
                                      TSlideHandleList &outChildren) const
{
    transformv_all(GetSlideNF(inSlide, m_Objects)->m_Children, outChildren);
}

int CSimpleSlideCore::GetChildIndex(CUICDMSlideHandle inParent, CUICDMSlideHandle inChild) const
{
    const SSlide *theSlide = GetSlideNF(inParent, m_Objects);
    size_t dist = distance(theSlide->m_Children.begin(),
                           find_if<TIntList::const_iterator>(
                               theSlide->m_Children, std::bind(equal_to<int>(), inChild,
                                                               std::placeholders::_1)));
    if (dist == theSlide->m_Children.size())
        throw SlideChildNotFoundError(L"");
    return (int)dist;
}

bool CSimpleSlideCore::GetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                                CUICDMInstanceHandle inHandle,
                                                CUICDMPropertyHandle inProperty,
                                                SValue &outValue) const
{
    const SSlide *theSlide = GetSlideNF(inSlide, m_Objects);
    SInternValue *theValue = theSlide->GetInstancePropertyValue(inHandle, inProperty);
    if (theValue) {
        outValue = theValue->GetValue();
        return true;
    }
    if (theSlide->m_Parent)
        return GetInstancePropertyValue(theSlide->m_Parent, inHandle, inProperty, outValue);
    return false;
}

std::pair<SSlide *, SInternValue *> CSimpleSlideCore::ResolveSetInstancePropertyValue(
    CUICDMSlideHandle inSlide, CUICDMInstanceHandle inHandle, CUICDMPropertyHandle inProperty)
{
    SSlide *theSlide = CSimpleSlideCore::GetSlideNF(inSlide, m_Objects);
    SInternValue *theValue = theSlide->GetInstancePropertyValue(inHandle, inProperty);
    // If we have the value already *or* or parent is not a valid slide, then return now
    if (theValue || theSlide->m_Parent == 0)
        return std::make_pair(theSlide, theValue);
    // Else give our parent a chance.
    return ResolveSetInstancePropertyValue(theSlide->m_Parent, inHandle, inProperty);
}

void CSimpleSlideCore::SetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                                CUICDMInstanceHandle inHandle,
                                                CUICDMPropertyHandle inProperty,
                                                const SValue &inValue)
{
    std::pair<SSlide *, SInternValue *> theTarget(
        ResolveSetInstancePropertyValue(inSlide, inHandle, inProperty));
    SInternValue theValue(inValue, GetStringTable());
    if (theTarget.second)
        *theTarget.second = theValue;
    else
        theTarget.first->SetInstancePropertyValue(inHandle, inProperty, theValue);
}

void CSimpleSlideCore::ForceSetInstancePropertyValue(CUICDMSlideHandle inSlide,
                                                     CUICDMInstanceHandle inHandle,
                                                     CUICDMPropertyHandle inProperty,
                                                     const SValue &inValue)
{
    CSimpleSlideCore::ForceSetPropertyValue(GetStringTable(), m_Objects, inSlide, inHandle,
                                            inProperty, inValue);
}

bool CSimpleSlideCore::GetSpecificInstancePropertyValue(CUICDMSlideHandle inSlide,
                                                        CUICDMInstanceHandle inInstance,
                                                        CUICDMPropertyHandle inProperty,
                                                        SValue &outValue) const
{
    const SSlide *theSlide = GetSlideNF(inSlide, m_Objects);
    SInternValue *theValue = theSlide->GetInstancePropertyValue(inInstance, inProperty);
    if (theValue) {
        outValue = theValue->GetValue();
        return true;
    }
    return false;
}

void CSimpleSlideCore::GetSpecificInstancePropertyValues(CUICDMSlideHandle inSlide,
                                                         CUICDMInstanceHandle inInstance,
                                                         TPropertyHandleValuePairList &outValues)
{
    const SSlide *theSlide = GetSlideNF(inSlide, m_Objects);
    theSlide->GetSpecificInstancePropertyValues(inInstance, outValues);
}

void CSimpleSlideCore::GetSlidePropertyEntries(CUICDMSlideHandle inSlide,
                                               TSlideEntryList &outEntries) const
{
    const SSlide *theSlide = GetSlideNF(inSlide, m_Objects);
    theSlide->ToSlideEntryList(outEntries);
}

bool CSimpleSlideCore::ContainsProperty(CUICDMSlideHandle inSlide, CUICDMInstanceHandle inHandle,
                                        CUICDMPropertyHandle inProperty) const
{
    const SSlide *theSlide = GetSlideNF(inSlide, m_Objects);
    return theSlide->GetInstancePropertyValue(inHandle, inProperty) != NULL;
}

CUICDMSlideHandle CSimpleSlideCore::CreateSlideWithHandle(int inHandle,
                                                          CUICDMInstanceHandle inInstance)
{
    if (HandleValid(inHandle))
        throw HandleExists(L"");
    m_Objects.insert(make_pair(inHandle, (THandleObjectPtr) new SSlide(inHandle, inInstance)));
    return inHandle;
}

void CSimpleSlideCore::GetSlideProperties(CUICDMSlideHandle inSlide,
                                          TSlideEntryList &outProperties) const
{
    outProperties.clear();
    GetSlidePropertyEntries(inSlide, outProperties);
}

bool CSimpleSlideCore::IsSlide(CUICDMSlideHandle inSlide) const
{
    return m_Objects.find(inSlide) != m_Objects.end();
}

void CSimpleSlideCore::ForceSetPropertyValue(IStringTable &inStringTable,
                                             THandleObjectMap &inObjects, CUICDMSlideHandle inSlide,
                                             CUICDMInstanceHandle inHandle,
                                             CUICDMPropertyHandle inProperty, const SValue &inValue)
{
    CSimpleSlideCore::GetSlideNF(inSlide, inObjects)
        ->SetInstancePropertyValue(inHandle, inProperty, SInternValue(inValue, inStringTable));
}

void CSimpleSlideCore::PushPropertyValueToChildren(CUICDMSlideHandle inParent,
                                                   CUICDMInstanceHandle inHandle,
                                                   CUICDMPropertyHandle inProperty,
                                                   const SValue &inValue)
{
    ForceSetPropertyValue(GetStringTable(), m_Objects, inParent, inHandle, inProperty, inValue);
    do_all(CSimpleSlideCore::GetSlideNF(inParent, m_Objects)->m_Children,
           std::bind(ForceSetPropertyValue, std::ref(GetStringTable()), std::ref(m_Objects),
                       std::placeholders::_1, inHandle, inProperty, inValue));
}

inline void AddIntersectingEntry(TSlideEntryList &outEntries, CUICDMInstanceHandle inst,
                                 CUICDMPropertyHandle prop, const SInternValue &inValue)
{
    outEntries.push_back(TSlideEntry(inst, prop, inValue.GetValue()));
}

void CSimpleSlideCore::GetIntersectingProperties(CUICDMSlideHandle inSlide1,
                                                 CUICDMSlideHandle inSlide2,
                                                 TSlideEntryList &outEntries) const
{
    const SSlide *theSlide1 = GetSlideNF(inSlide1, m_Objects);
    const SSlide *theSlide2 = GetSlideNF(inSlide2, m_Objects);
    theSlide1->IntersectProperties(
        *theSlide2, std::bind(AddIntersectingEntry, std::ref(outEntries),
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
}

void CSimpleSlideCore::PushIntersectingProperties(CUICDMSlideHandle inSlide1,
                                                  CUICDMSlideHandle inSlide2,
                                                  CUICDMSlideHandle inDestination)
{
    const SSlide *theSlide1 = GetSlideNF(inSlide1, m_Objects);
    const SSlide *theSlide2 = GetSlideNF(inSlide2, m_Objects);
    SSlide *theDest = GetSlideNF(inDestination, m_Objects);
    theSlide1->IntersectProperties(
        *theSlide2, std::bind(&SSlide::SetInstancePropertyValue, theDest,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3));
}

void CSimpleSlideCore::ClearPropertyValue(THandleObjectMap &inObjects, CUICDMSlideHandle inSlide,
                                          CUICDMInstanceHandle inInstance,
                                          CUICDMPropertyHandle inProperty)
{
    CSimpleSlideCore::GetSlideNF(inSlide, inObjects)
        ->RemoveInstancePropertyValue(inInstance, inProperty);
}

inline void DoForEachSlide(std::pair<int, THandleObjectPtr> inObject,
                           std::function<void(SSlide *)> inFunction)
{
    inFunction((SSlide *)inObject.second.get());
}

void CSimpleSlideCore::ForEachSlide(std::function<void(SSlide *)> inFunction)
{
    do_all(m_Objects, std::bind(DoForEachSlide, std::placeholders::_1, inFunction));
}

void LookupSlideAndDoSomething(CUICDMSlideHandle inSlide, THandleObjectMap &inObjects,
                               std::function<void(SSlide *)> inFunction)
{
    inFunction(CSimpleSlideCore::GetSlideNF(inSlide, inObjects));
}

void CSimpleSlideCore::ForEachChild(CUICDMSlideHandle inParent,
                                    std::function<void(SSlide *)> inFunction)
{
    do_all(GetSlideNF(inParent, m_Objects)->m_Children,
           std::bind(LookupSlideAndDoSomething, std::placeholders::_1, m_Objects, inFunction));
}

bool InstanceMatches(CUICDMInstanceHandle inTarget, CUICDMInstanceHandle inHandle,
                     CUICDMPropertyHandle)
{
    return inTarget == inHandle;
}

bool PropertyMatches(CUICDMPropertyHandle inTarget, CUICDMInstanceHandle,
                     CUICDMPropertyHandle inProp)
{
    return inTarget == inProp;
}

bool InstancePropertyMatchesVector(const TInstanceHandleList &inInstances,
                                   const TPropertyHandleList &inProperties,
                                   CUICDMInstanceHandle slideInst, CUICDMPropertyHandle slideProp)
{
    return std::find(inInstances.begin(), inInstances.end(), slideInst) != inInstances.end()
        && std::find(inProperties.begin(), inProperties.end(), slideProp) != inProperties.end();
}

bool InstancePropertyMatches(const CUICDMInstanceHandle inInstance,
                             const CUICDMPropertyHandle inProperty, CUICDMInstanceHandle slideInst,
                             CUICDMPropertyHandle slideProp)
{
    return inInstance == slideInst && inProperty == slideProp;
}

void CSimpleSlideCore::DeleteAllInstanceEntries(CUICDMInstanceHandle inHandle)
{
    std::function<bool(CUICDMInstanceHandle, CUICDMPropertyHandle)> predicate(
        std::bind(InstanceMatches, inHandle, std::placeholders::_1, std::placeholders::_2));
    ForEachSlide(std::bind(&SSlide::ClearPropertiesIf, std::placeholders::_1, predicate));
}

void CSimpleSlideCore::DeleteAllPropertyEntries(CUICDMPropertyHandle inHandle)
{
    std::function<bool(CUICDMInstanceHandle, CUICDMPropertyHandle)> predicate(
        std::bind(PropertyMatches, inHandle, std::placeholders::_1, std::placeholders::_2));
    ForEachSlide(std::bind(&SSlide::ClearPropertiesIf, std::placeholders::_1, predicate));
}

void CSimpleSlideCore::DeleteAllInstancePropertyEntries(const TInstanceHandleList &inInstances,
                                                        const TPropertyHandleList &inProperties)
{
    std::function<bool(CUICDMInstanceHandle, CUICDMPropertyHandle)> predicate(
        std::bind(InstancePropertyMatchesVector, inInstances, inProperties,
                  std::placeholders::_1, std::placeholders::_2));
    ForEachSlide(std::bind(&SSlide::ClearPropertiesIf, std::placeholders::_1, predicate));
}

void CSimpleSlideCore::ClearChildrenPropertyValues(CUICDMSlideHandle inParent,
                                                   CUICDMInstanceHandle inHandle,
                                                   CUICDMPropertyHandle inProperty)
{
    std::function<bool(CUICDMInstanceHandle, CUICDMPropertyHandle)> predicate(
        std::bind(InstancePropertyMatches, inHandle, inProperty,
                  std::placeholders::_1, std::placeholders::_2));
    ForEachChild(inParent, std::bind(&SSlide::ClearPropertiesIf, std::placeholders::_1, predicate));
}
}
