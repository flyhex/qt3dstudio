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

#include "Qt3DSCommonPrecompile.h"
#include "Qt3DSDMInspectable.h"
#include "Qt3DSDMInspectorGroup.h"
#include "Qt3DSDMInspectorRow.h"
#include "StudioApp.h"
#include "Doc.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMDataCore.h"
#include "Qt3DSDMPropertyDefinition.h"
#include "Core.h"
#include "StudioFullSystem.h"
#include "StudioCoreSystem.h"
#include "Qt3DSDMMetaData.h"
#include "Qt3DSDMSlides.h"
#include "IDocumentReader.h"

using namespace qt3dsdm;

//==============================================================================
/**
 *	Constructor
 */
Qt3DSDMInspectable::Qt3DSDMInspectable(CStudioApp &inApp, CCore *inCore,
                                       qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                       qt3dsdm::Qt3DSDMInstanceHandle inDualPersonalityInstance)
    : CInspectableBase(inCore)
    , m_Instance(inInstance)
    , m_DualPersonalityInstance((inDualPersonalityInstance != 0) ? inDualPersonalityInstance
                                                                 : inInstance)
    , m_App(inApp)
{
    QT3DS_ASSERT(inCore->GetDoc()->GetDocumentReader().IsInstance(m_Instance));
}

//==============================================================================
/**
 *	Query the name of the inspectable item
 */
QString Qt3DSDMInspectable::GetName()
{
    CClientDataModelBridge *theBridge =
            m_Core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();

    if (m_Instance == m_DualPersonalityInstance)
        return theBridge->GetName(m_Instance);

    QString theName = theBridge->GetName(m_Instance);
    theName += " (";
    theName += theBridge->GetName(m_DualPersonalityInstance);
    theName += ")";

    return theName;
}

//==============================================================================
/**
 *	Query the number of groups to display
 */
long Qt3DSDMInspectable::GetGroupCount()
{
    // If you have a dual personality inspectable then you may overwrite
    QT3DS_ASSERT(
                m_Instance == m_DualPersonalityInstance
                || m_Core->GetDoc()->GetStudioSystem()->GetClientDataModelBridge()
                ->IsComponentInstance(m_Instance));
    IMetaData &theMetaData = *m_Core->GetDoc()->GetStudioSystem()->GetActionMetaData();
    long count = (long)theMetaData.GetGroupCountForInstance(m_Instance);
    return count;
}

//==============================================================================
/**
 *	Return the property group for display
 */
CInspectorGroup *Qt3DSDMInspectable::GetGroup(long inIndex)
{
    Qt3DSDMInspectorGroup *theGroup =
            new Qt3DSDMInspectorGroup(m_App, GetGroupName(inIndex), *this, inIndex);

    TMetaDataPropertyHandleList theProperties = GetGroupProperties(inIndex);

    size_t thePropertyCount = theProperties.size();
    for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount; ++thePropertyIndex)
        theGroup->CreateRow(m_Core->GetDoc(), theProperties[thePropertyIndex]);

    return theGroup;
}

//==============================================================================
/**
 *	Return the property handles for display, given the group index
 */
TMetaDataPropertyHandleList Qt3DSDMInspectable::GetGroupProperties(long inIndex)
{
    TMetaDataPropertyHandleList retval;
    IMetaData &theMetaData = *m_Core->GetDoc()->GetStudioSystem()->GetActionMetaData();
    theMetaData.GetMetaDataProperties(GetGroupInstance(inIndex), retval);
    qt3dsdm::IPropertySystem &thePropertySystem(
                *m_Core->GetDoc()->GetStudioSystem()->GetPropertySystem());
    // get name of the current group fofr filtering
    Option<QString> theGroupFilterName =
            theMetaData.GetGroupFilterNameForInstance(GetGroupInstance(inIndex), inIndex);
    long theGroupCount = GetGroupCount();

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
                    bool resultIfTrue =
                            theFilter.m_FilterType == PropertyFilterTypes::ShowIfEqual
                            ? true : false;
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

//==============================================================================
/**
 *	Return the Resource String ID for the Group Name, given the group index
 */
QString Qt3DSDMInspectable::GetGroupName(long inGroupIndex)
{
    std::vector<QString> theGroupNames;
    IMetaData &theMetaData = *m_Core->GetDoc()->GetStudioSystem()->GetActionMetaData();
    theMetaData.GetGroupNamesForInstance(GetGroupInstance(inGroupIndex), theGroupNames);

    size_t theIndex = inGroupIndex;

    if (theGroupNames.size() > theIndex)
        return theGroupNames[inGroupIndex];
    return QObject::tr("Basic Properties");
}

//==============================================================================
/**
 *	Return the Inspectable Instance Handle for the Group, given the group index
 */
Qt3DSDMInstanceHandle Qt3DSDMInspectable::GetGroupInstance(long inGroupIndex)
{
    Q_UNUSED(inGroupIndex);
    return m_DualPersonalityInstance;
}

EStudioObjectType Qt3DSDMInspectable::GetObjectType()
{
    IMetaData &theMetaData = *m_Core->GetDoc()->GetStudioSystem()->GetActionMetaData();
    Option<QString> theObjTypeName = theMetaData.GetTypeForInstance(m_Instance);
    if (theObjTypeName.hasValue()) {
        ComposerObjectTypes::Enum theType =
                ComposerObjectTypes::Convert(theObjTypeName);
        switch (theType) {
        case ComposerObjectTypes::Slide: {
            CDoc *theDoc = m_Core->GetDoc();
            CClientDataModelBridge *theBridge =
                    theDoc->GetStudioSystem()->GetClientDataModelBridge();
            qt3dsdm::Qt3DSDMInstanceHandle theInstance =
                    theBridge->GetOwningComponentInstance(theDoc->GetActiveSlide());
            Option<QString> theObjTypeName = theMetaData.GetTypeForInstance(theInstance);
            if (theObjTypeName.hasValue()) {
                ComposerObjectTypes::Enum theType =
                        ComposerObjectTypes::Convert(theObjTypeName);
                if (theType == ComposerObjectTypes::Scene)
                    return OBJTYPE_SCENE;
                else
                    return OBJTYPE_COMPONENT;
            }
            return OBJTYPE_UNKNOWN;
        }
        case ComposerObjectTypes::Scene:
            return OBJTYPE_SCENE;
        case ComposerObjectTypes::Layer:
            return OBJTYPE_LAYER;
        case ComposerObjectTypes::Behavior:
            return OBJTYPE_BEHAVIOR;
        case ComposerObjectTypes::Material:
            return OBJTYPE_MATERIAL;
        case ComposerObjectTypes::Camera:
            return OBJTYPE_CAMERA;
        case ComposerObjectTypes::Light:
            return OBJTYPE_LIGHT;
        case ComposerObjectTypes::Model:
            return OBJTYPE_MODEL;
        case ComposerObjectTypes::Group:
            return OBJTYPE_GROUP;
        case ComposerObjectTypes::Image:
            return OBJTYPE_IMAGE;
        case ComposerObjectTypes::Text:
            return OBJTYPE_TEXT;
        case ComposerObjectTypes::Component:
            return OBJTYPE_COMPONENT;
        case ComposerObjectTypes::Effect:
            return OBJTYPE_EFFECT;
        case ComposerObjectTypes::CustomMaterial:
            return OBJTYPE_CUSTOMMATERIAL;
        case ComposerObjectTypes::ReferencedMaterial:
            return OBJTYPE_REFERENCEDMATERIAL;
        case ComposerObjectTypes::Path:
            return OBJTYPE_PATH;
        case ComposerObjectTypes::SubPath:
            return OBJTYPE_SUBPATH;
        case ComposerObjectTypes::PathAnchorPoint:
            return OBJTYPE_PATHANCHORPOINT;
        case ComposerObjectTypes::Lightmaps:
            return OBJTYPE_LIGHTMAPS;
        default:
            break;
        }
    }
    return OBJTYPE_UNKNOWN;
}

bool Qt3DSDMInspectable::IsValid() const
{
    return m_Core->GetDoc()->GetStudioSystem()->IsInstance(m_Instance)
            && m_Core->GetDoc()->GetStudioSystem()->IsInstance(m_DualPersonalityInstance);
}

bool Qt3DSDMInspectable::IsMaster()
{
    ISlideSystem *theSlideSystem = m_Core->GetDoc()->GetStudioSystem()->GetSlideSystem();
    qt3dsdm::Qt3DSDMSlideHandle theSlideHandle = theSlideSystem->GetAssociatedSlide(m_Instance);
    if (theSlideHandle.Valid())
        return theSlideSystem->IsMasterSlide(theSlideHandle);
    // Slide handle may not be valid if we are selecting the Scene or if we are inside Component and
    // we select the Component root.
    return false;
}
