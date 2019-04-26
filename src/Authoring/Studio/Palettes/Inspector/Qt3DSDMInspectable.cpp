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

#include "Qt3DSDMInspectable.h"
#include "Qt3DSDMInspectorGroup.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMSlides.h"
#include "IDocumentReader.h"

using namespace qt3dsdm;

Qt3DSDMInspectable::Qt3DSDMInspectable(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                       qt3dsdm::Qt3DSDMInstanceHandle activeSlideInstance)
    : m_instance(instance)
    , m_activeSlideInstance(activeSlideInstance)
{
    QT3DS_ASSERT(getDoc()->GetDocumentReader().IsInstance(m_instance));

    if (m_activeSlideInstance) {
        // only active root scene or components set m_activeSlideInstance
        auto *bridge = getDoc()->GetStudioSystem()->GetClientDataModelBridge();
        QT3DS_ASSERT(bridge->IsSceneInstance(instance)
                     || bridge->IsComponentInstance(instance));
    }
}

// Returns the name of this inspectable
Q3DStudio::CString Qt3DSDMInspectable::getName()
{
    auto *bridge = getDoc()->GetStudioSystem()->GetClientDataModelBridge();

    if (!m_activeSlideInstance)
        return bridge->GetName(m_instance, true);

    Q3DStudio::CString theName = bridge->GetName(m_instance, true);
    theName += " (";
    theName += bridge->GetName(m_activeSlideInstance, true);
    theName += ")";

    return theName;
}

// Returns the number of groups in this inspectable
long Qt3DSDMInspectable::getGroupCount() const
{
    IMetaData &theMetaData = *getDoc()->GetStudioSystem()->GetActionMetaData();
    // In addition to a background group, Scene has a basic properties group (hidden in
    // inspector) because it is derived from Asset. Until this is fixed properly, we force the
    // Scene groups count to 1 (else an empty group will appear in the inspector).
    long count = getObjectType() == OBJTYPE_SCENE ? 1
                                    : long(theMetaData.GetGroupCountForInstance(m_instance));

    if (m_activeSlideInstance)
        count += long(theMetaData.GetGroupCountForInstance(m_activeSlideInstance));

    return count;
}

// Return the property group for display
CInspectorGroup *Qt3DSDMInspectable::getGroup(long inIndex)
{
    Qt3DSDMInspectorGroup *group = new Qt3DSDMInspectorGroup(GetGroupName(inIndex));

    TMetaDataPropertyHandleList properties = GetGroupProperties(inIndex);

    for (auto &prop : properties)
        group->CreateRow(getDoc(), prop);

    return group;
}

// Return the property handles for display, given the group index
TMetaDataPropertyHandleList Qt3DSDMInspectable::GetGroupProperties(long inIndex)
{
    long activeGroupIdx = activeGroupIndex(inIndex);
    TMetaDataPropertyHandleList retval;
    IMetaData &theMetaData = *getDoc()->GetStudioSystem()->GetActionMetaData();
    theMetaData.GetMetaDataProperties(GetGroupInstance(inIndex), retval);
    qt3dsdm::IPropertySystem &thePropertySystem(*getDoc()->GetStudioSystem()->GetPropertySystem());
    // get name of the current group for filtering
    Option<qt3dsdm::TCharStr> theGroupFilterName =
            theMetaData.GetGroupFilterNameForInstance(GetGroupInstance(inIndex), activeGroupIdx);
    long theGroupCount = getGroupCount();

    // end is explicitly required
    for (size_t idx = 0; idx < retval.size(); ++idx) {
        if (theMetaData.GetMetaDataPropertyInfo(retval[idx])->m_IsHidden) {
            retval.erase(retval.begin() + idx);
            --idx;
        } else if (theGroupCount > 1 && theGroupFilterName.hasValue()
                   && theMetaData.GetMetaDataPropertyInfo(retval[idx])->m_GroupName
                   != theGroupFilterName) {
            retval.erase(retval.begin() + idx);
            --idx;
        } else {
            qt3ds::foundation::NVConstDataRef<SPropertyFilterInfo> theFilters(
                        theMetaData.GetMetaDataPropertyFilters(retval[idx]));
            if (theFilters.size()) {
                Option<bool> keepProperty;
                // The tests are done in an ambiguous way.  Really, show if equal should take
                // multiple conditions
                // as should hide if equal.  They do not, so we need to rigorously define exactly
                // how those two interact.
                for (QT3DSU32 propIdx = 0, propEnd = theFilters.size(); propIdx < propEnd;
                     ++propIdx) {
                    const SPropertyFilterInfo &theFilter(theFilters[propIdx]);

                    QT3DS_ASSERT(theFilter.m_FilterType == PropertyFilterTypes::ShowIfEqual
                                 || theFilter.m_FilterType == PropertyFilterTypes::HideIfEqual);

                    SValue theValue;
                    thePropertySystem.GetInstancePropertyValue(
                                GetGroupInstance(inIndex), theFilter.m_FilterProperty, theValue);
                    bool resultIfTrue = theFilter.m_FilterType == PropertyFilterTypes::ShowIfEqual;
                    if (Equals(theValue.toOldSkool(), theFilter.m_Value.toOldSkool())) {
                        keepProperty = resultIfTrue;
                        break;
                    } else {
                        keepProperty = !resultIfTrue;
                    }
                }
                if (keepProperty.hasValue() && *keepProperty == false) {
                    retval.erase(retval.begin() + idx);
                    --idx;
                }
            }
        }
    }
    return retval;
}

// Return the Group Name, given the group index
QString Qt3DSDMInspectable::GetGroupName(long groupIndex)
{
    std::vector<TCharStr> theGroupNames;
    IMetaData &theMetaData = *getDoc()->GetStudioSystem()->GetActionMetaData();
    theMetaData.GetGroupNamesForInstance(GetGroupInstance(groupIndex), theGroupNames);

    long activeGroupIdx = activeGroupIndex(groupIndex);
    if (activeGroupIdx < theGroupNames.size())
        return Q3DStudio::CString(theGroupNames[activeGroupIdx].wide_str()).toQString();

    return QObject::tr("Basic Properties");
}

// Return the Inspectable Instance Handle for the Group, given the group index
Qt3DSDMInstanceHandle Qt3DSDMInspectable::GetGroupInstance(long inGroupIndex)
{
    // if active root, return the slide instance at first index
    if (m_activeSlideInstance && inGroupIndex == 0)
        return m_activeSlideInstance;

    return m_instance;
}

EStudioObjectType Qt3DSDMInspectable::getObjectType() const
{
    return getDoc()->GetStudioSystem()->GetClientDataModelBridge()->GetObjectType(m_instance);
}

bool Qt3DSDMInspectable::isValid() const
{
    if (m_activeSlideInstance) {
        return getDoc()->GetStudioSystem()->IsInstance(m_instance)
                && getDoc()->GetStudioSystem()->IsInstance(m_activeSlideInstance);
    }
    return getDoc()->GetStudioSystem()->IsInstance(m_instance);
}

bool Qt3DSDMInspectable::isMaster() const
{
    ISlideSystem *slideSystem = getDoc()->GetStudioSystem()->GetSlideSystem();
    qt3dsdm::Qt3DSDMSlideHandle theSlideHandle = slideSystem->GetAssociatedSlide(m_instance);
    if (theSlideHandle.Valid())
        return slideSystem->IsMasterSlide(theSlideHandle);
    // Slide handle may not be valid if we are selecting the Scene or if we are inside Component and
    // we select the Component root.
    return false;
}

// Returns the group index taking into consideration that for active roots, first index is the slide
// group so need to decrement all index bigger than 1, by 1. For scene we decrement 1 more because
// the first group (Basic properties) is not in use.
long Qt3DSDMInspectable::activeGroupIndex(long groupIndex) const
{
    if (m_activeSlideInstance && groupIndex > 0 && getObjectType() != OBJTYPE_SCENE)
        return groupIndex - 1;

    return groupIndex;
}

CDoc *Qt3DSDMInspectable::getDoc() const
{
    return g_StudioApp.GetCore()->GetDoc();
}
