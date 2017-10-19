/****************************************************************************
**
** Copyright (C) 1999-2005 Anark Corporation.
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
#pragma once
#ifndef IDOCUMENTREADERH
#define IDOCUMENTREADERH
#include "Qt3DSDMDataTypes.h"
#include "foundation/Qt3DSOption.h"
#include <vector>
#include <utility>
#include "StudioObjectTypes.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "Qt3DSFileTools.h"

namespace qt3dsdm {
class IDOMWriter;
class IDOMReader;
struct SGuideInfo;
}

namespace qt3dsimp {
class ITranslator;
struct STranslationLog;
}

namespace qt3ds {
class NVFoundationBase;
}

namespace Q3DStudio {
using std::shared_ptr;
class IDynamicLua;

typedef std::vector<std::pair<qt3dsdm::Qt3DSDMSlideHandle, qt3dsdm::Qt3DSDMInstanceHandle>>
    TSlideInstanceList;
typedef std::vector<std::pair<qt3dsdm::Qt3DSDMSlideHandle, Q3DStudio::CString>> TSlideStringList;
typedef std::unordered_map<const wchar_t *, TSlideInstanceList> TCharPtrToSlideInstanceMap;
using qt3ds::foundation::Option;
using qt3dsdm::Qt3DSDMInstanceHandle;
using qt3dsdm::Qt3DSDMPropertyHandle;
using qt3dsdm::Qt3DSDMSlideHandle;
using qt3dsdm::CUICDMKeyframeHandle;
using qt3dsdm::CUICDMActionHandle;
using qt3dsdm::CUICDMAnimationHandle;
using qt3dsdm::SValue;
using qt3dsdm::IDOMReader;
using qt3dsdm::IDOMWriter;

// Reader class to read complex information from the document.  Encapsulates the various UICDM
// subsystems
// Implemented in DocumentEditor.cpp
class IDocumentReader
{
protected:
    virtual ~IDocumentReader() {}
public:
    typedef Qt3DSDMInstanceHandle TInstanceHandle;
    typedef Qt3DSDMPropertyHandle TPropertyHandle;
    typedef Qt3DSDMSlideHandle TSlideHandle;
    typedef CUICDMKeyframeHandle TKeyframeHandle;
    typedef qt3dsdm::SValue SValue;
    typedef std::pair<TSlideHandle, SValue> TSlideValuePair;
    typedef std::vector<TSlideValuePair> TSlideValuePairList;
    typedef std::vector<TSlideHandle> TSlideList;
    typedef std::vector<TInstanceHandle> TInstanceList;
    typedef std::vector<TPropertyHandle> TPropertyList;

    // Returns true if this handle points to a valid instance.
    virtual bool IsInstance(TInstanceHandle inInstance) const = 0;
    virtual bool IsCurrentlyActive(TInstanceHandle inInstance) const = 0;
    // Find a property on an instance by name.  Searches derivation parents as well
    virtual TPropertyHandle FindProperty(TInstanceHandle instance, const wchar_t *inName) const = 0;
    // Get the instance property value by going through the current slide
    // This will convert image properties to source paths.  If this is not desired, then use
    virtual Option<SValue> GetInstancePropertyValue(TInstanceHandle instance,
                                                    TPropertyHandle inProperty) const = 0;
    // This will not convert image properties to source paths, they will come across as guids.
    virtual Option<SValue> GetRawInstancePropertyValue(TInstanceHandle instance,
                                                       TPropertyHandle inProperty) const = 0;
    // Get an image instance by dereferencing this property pair
    virtual TInstanceHandle GetImageInstanceForProperty(TInstanceHandle instance,
                                                        TPropertyHandle inProperty) const = 0;
    // Get the property value for this instance from this slide without doing normal slide lookup.
    // If inSlide isn't valid, returns the property value for this instance directly from the data
    // core.
    virtual Option<SValue> GetSpecificInstancePropertyValue(TSlideHandle inSlide,
                                                            TInstanceHandle instance,
                                                            TPropertyHandle inProperty) const = 0;

    virtual Q3DStudio::CString GetObjectTypeName(TInstanceHandle instance) const = 0;

    virtual CString GetName(Qt3DSDMInstanceHandle inInstance) const = 0;
    virtual TInstanceHandle GetFirstBaseClass(Qt3DSDMInstanceHandle inInstance) const = 0;
    virtual CString GetSourcePath(Qt3DSDMInstanceHandle inInstance) const = 0;

    template <typename TDataType>
    inline Option<TDataType> GetTypedInstancePropertyValue(TInstanceHandle instance,
                                                           TPropertyHandle inProperty);
    // Get every property value associated with this instance, from the data core up.  The
    // associated slide will be NULL for the
    // data core.
    virtual void GetAllPropertyValues(TInstanceHandle instance, TPropertyHandle inProperty,
                                      TSlideValuePairList &outList) const = 0;
    // Get the slide this instance was created under
    virtual TSlideHandle GetAssociatedSlide(TInstanceHandle inInstance) const = 0;
    // Is this a master slide?
    virtual bool IsMasterSlide(TSlideHandle inSlide) const = 0;
    // Get the component this instance was created under.
    virtual TInstanceHandle GetAssociatedComponent(TInstanceHandle inInstance) const = 0;
    // Get the slide that is currently active for this instance in the UI.
    virtual TSlideHandle GetActiveSlide(TInstanceHandle inInstance) const = 0;
    // Return the active slide for the given component.
    virtual TSlideHandle GetComponentActiveSlide(TInstanceHandle inInstance) const = 0;
    // Get the component who owns a given slide.
    virtual TInstanceHandle GetComponentForSlide(TSlideHandle inSlide) const = 0;
    // Get every slide, starting with the associated slide (the master slide) that
    // this instance is associated with.  So the first slide will be the slide this
    // instance was created under, then next slides will be that associated slide's
    // children.
    virtual void GetAllAssociatedSlides(TInstanceHandle inInstance, TSlideList &outList) const = 0;

    // Returns true if this object is the scene or a components
    virtual bool IsComponent(TInstanceHandle inInstance) const = 0;

    // Get all the source paths for this instance.  If the slide is zero, this means the source
    // path is set in the data core.  Else this indicates exactly which slide the source path is
    // set under.
    virtual void GetAllSourcePaths(TInstanceHandle inInstance,
                                   TSlideStringList &outPaths) const = 0;

    // Get a map from string table string (datacore.GetStringTable().RegisterStr()) to
    // a list of slide/instance pairs that indicate, for a given source path, which instances
    // link to that source path and under which slide.
    // Pass in true if you want the map keys to include the identifiers (#1, #2).
    virtual void GetSourcePathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap,
                                            bool inIncludeIdentifiers = true) const = 0;
    // Get a map from string table string to list of slide/instance pairs that indicates, for a
    // given source path,
    // which instances link to that source path under which slide.
    virtual void GetImportPathToInstanceMap(TCharPtrToSlideInstanceMap &outInstanceMap) const = 0;

    // Return true if a property is linked (exists only on the associated slide).
    virtual bool CanPropertyBeLinked(TInstanceHandle inInstance,
                                     TPropertyHandle inProperty) const = 0;
    virtual bool IsPropertyLinked(TInstanceHandle inInstance, TPropertyHandle inProperty) const = 0;

    // If this property is linked, return the associated slide for the instance.
    // If this property is unlinked, return the active slide.
    virtual TSlideHandle GetSlideForProperty(TInstanceHandle inInstance,
                                             TPropertyHandle inProperty) const = 0;
    // Return true if this instance is an imported instance.
    virtual bool IsImported(TInstanceHandle inInstance) const = 0;
    virtual CString GetImportId(TInstanceHandle inInstance) const = 0;
    virtual CString GetFileId(TInstanceHandle inInstance) const = 0;

    // Return (start,end), inclusive, that represent this instance's lifetime.
    virtual std::pair<long, long> GetTimeRange(TInstanceHandle instance) const = 0;
    virtual std::pair<long, long> GetTimeRangeInSlide(Qt3DSDMSlideHandle inSlide,
                                                      TInstanceHandle instance) const = 0;
    // Get the GUID for a given instance.
    virtual qt3dsdm::SLong4 GetGuidForInstance(TInstanceHandle instance) const = 0;
    virtual TInstanceHandle GetInstanceForGuid(const qt3dsdm::SLong4 &inGuid) const = 0;
    virtual TInstanceHandle
    GetInstanceForObjectRef(TInstanceHandle inRootInstance,
                            const qt3dsdm::SObjectRefType &inReference) const = 0;
    // Get the scene graph parent of this instance
    virtual TInstanceHandle GetParent(TInstanceHandle child) const = 0;
    // Get all the children if this instance in this slide.  If the slide is invalid,
    // the get all the children of this parent in all slides.
    virtual void GetChildren(TSlideHandle inSlide, TInstanceHandle inParent,
                             TInstanceList &outChildren) const = 0;
    // Returns true if this instance is anywhere the scene graph
    virtual bool IsInSceneGraph(TInstanceHandle child) const = 0;

    virtual bool IsPathExternalizeable(TInstanceHandle path) const = 0;
    virtual bool IsPathInternalizeable(TInstanceHandle path) const = 0;

    virtual bool AnimationExists(TSlideHandle inSlide, TInstanceHandle instance,
                                 const wchar_t *propName, long subIndex) = 0;
    virtual bool IsAnimationArtistEdited(TSlideHandle inSlide, TInstanceHandle instance,
                                         const wchar_t *propName, long subIndex) = 0;

    virtual qt3dsdm::TGuideHandleList GetGuides() const = 0;
    virtual qt3dsdm::SGuideInfo GetGuideInfo(qt3dsdm::CUICDMGuideHandle inGuide) const = 0;
    virtual bool IsGuideValid(qt3dsdm::CUICDMGuideHandle inGuide) const = 0;
    virtual bool AreGuidesEditable() const = 0;
    // This can be done outside a transaction, hence it is an operation on the reader
    // not on the editor.
    virtual void SetGuidesEditable(bool inEditable) = 0;

    // Given the active slide on the document (which we query) and an instance,
    // produce a temporary file (CUICFile::GetTemporaryFile) and serialize this object
    // to that temporary file.
    virtual CFilePath CopySceneGraphObjects(qt3dsdm::TInstanceHandleList inInstances) = 0;

    CFilePath CopySceneGraphObject(Qt3DSDMInstanceHandle inInstance)
    {
        qt3dsdm::TInstanceHandleList theInstances;
        theInstances.push_back(inInstance);
        return CopySceneGraphObjects(theInstances);
    }

    // Copy the object just to a DOM representation, don't serialize to file.
    virtual std::shared_ptr<qt3dsdm::IDOMReader>
    CopySceneGraphObjectToMemory(Qt3DSDMInstanceHandle inInstance) = 0;

    virtual CFilePath CopyAction(CUICDMActionHandle inAction, Qt3DSDMSlideHandle inSlide) = 0;

    virtual void ParseSourcePathsOutOfEffectFile(Q3DStudio::CString inFile,
                                                 std::vector<Q3DStudio::CString> &outFilePaths) = 0;

    virtual Q3DStudio::CString GetCustomMaterialName(const Q3DStudio::CString &inFullPathToFile) const = 0;

    // Must not be a master slide.  Used during duplicate slide.
    virtual std::shared_ptr<qt3dsdm::IDOMReader> CopySlide(Qt3DSDMSlideHandle inSlide) = 0;

    // Don't release this, it is owned and cached by the document.  Also, reset top to 0 with a
    // scope, please.
    virtual IDynamicLua *GetLuaContext() = 0;
    virtual qt3ds::NVFoundationBase &GetFoundation() = 0;
};

template <typename TDataType>
inline Option<TDataType> IDocumentReader::GetTypedInstancePropertyValue(TInstanceHandle instance,
                                                                        TPropertyHandle inProperty)
{
    Option<SValue> theValue = GetInstancePropertyValue(instance, inProperty);
    if (theValue.isEmpty())
        return qt3ds::foundation::Empty();
    return qt3dsdm::get<TDataType>(*theValue);
}

// Helper function needed by a few objects
inline EStudioObjectType GetStudioObjectType(qt3dsdm::ComposerObjectTypes::Enum inType)
{
    using namespace qt3dsdm;
    switch (inType) {
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
    case ComposerObjectTypes::Path:
        return OBJTYPE_PATH;
    default: ;
    }
    return OBJTYPE_UNKNOWN;
}
}

#endif
