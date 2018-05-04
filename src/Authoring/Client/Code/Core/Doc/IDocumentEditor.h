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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_IDOCUMENTEDITOR_H
#define INCLUDED_IDOCUMENTEDITOR_H
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMAnimation.h"
#include "StudioObjectTypes.h"
#include "Pt.h"
#include "Qt3DSImportErrorCodes.h"
#include "DocumentEditorEnumerations.h"
#include "IDocumentReader.h"
#include "Qt3DSDMComposerTypeDefinitions.h"
#include "CColor.h"
#include "Qt3DSDMHandles.h"

#pragma once

class IDoc;
class CDoc;

namespace qt3dsdm {
class IDataCore;
class ISlideCore;
class ISlideSystem;
class IMetaData;
class SComposerObjectDefinitions;
struct SGuideInfo;
}

namespace qt3dsimp {
struct STranslationLog;
}

namespace qt3ds {
namespace render {
    class IInputStreamFactory;
    class ITextRenderer;
}
}

namespace Q3DStudio {
using std::wstring;
class CGraph;
class IInternalDocumentEditor;

class IImportFailedHandler
{
protected:
    virtual ~IImportFailedHandler() {}

public:
    virtual void DisplayImportFailed(const QString &inDocumentPath,
                                     const QString &inDescription,
                                     bool inWarningsOnly) = 0;
};

class IDeletingReferencedObjectHandler
{
protected:
    virtual ~IDeletingReferencedObjectHandler() {}

public:
    virtual void DisplayMessageBox(const Q3DStudio::CString &inDescription) = 0;
};

class IMoveRenameHandler
{
protected:
    virtual ~IMoveRenameHandler() {}

public:
    virtual void displayMessageBox(const Q3DStudio::CString &origName,
                                   const Q3DStudio::CString &newName) = 0;
};

class IDocumentEditor : public IDocumentReader
{
protected:
    virtual ~IDocumentEditor() {}

public:
    // Undo whatever has been done so far.
    virtual void Rollback() = 0;
    // Release when finished editing
    virtual void Release() = 0;

    // Notify the subsystem that we will be modifying a lot of properties and data
    // We filter events to only send one instance changed event per instance changed
    // no animation events and no keyframe events.
    virtual void BeginAggregateOperation() = 0;
    virtual void EndAggregateOperation() = 0;

    // Function callable without a document editor instance, used when loading document
    static TInstanceHandle
    CreateSceneGraphInstance(const wchar_t *inType, TInstanceHandle parent, TSlideHandle inSlide,
                             qt3dsdm::IDataCore &inDataCore, qt3dsdm::ISlideSystem &inSlideSystem,
                             qt3dsdm::SComposerObjectDefinitions &inObjectDefs,
                             Q3DStudio::CGraph &inAssetGraph, qt3dsdm::IMetaData &inMetaData,
                             TInstanceHandle inTargetId = TInstanceHandle());

    static TInstanceHandle CreateSceneGraphInstance(
        Qt3DSDMInstanceHandle inMaster, TInstanceHandle parent, TSlideHandle inSlide,
        qt3dsdm::IDataCore &inDataCore, qt3dsdm::ISlideSystem &inSlideSystem,
        qt3dsdm::SComposerObjectDefinitions &inObjectDefs, Q3DStudio::CGraph &inAssetGraph,
        qt3dsdm::IMetaData &inMetaData, TInstanceHandle inTargetId = TInstanceHandle());

    static void UnlinkAlwaysUnlinkedProperties(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                               qt3dsdm::SComposerObjectDefinitions &inDefs,
                                               qt3dsdm::ISlideSystem &inSlideSystem);

    // Returns valid properties followed by exactly one invalid property.
    static Qt3DSDMPropertyHandle *
    GetAlwaysUnlinkedProperties(qt3dsdm::SComposerObjectDefinitions &inDefs);

    // Create a new instance in the scene under this slide and such
    // Target id must be an id of an invalid instance so that we can potentially change an object
    // type while still maintaining references to that logical object.
    virtual TInstanceHandle
    CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Enum type, TInstanceHandle parent,
                             TSlideHandle inSlide,
                             TInstanceHandle inTargetId = TInstanceHandle()) = 0;

    virtual TInstanceHandle CreateSceneGraphInstance(qt3dsdm::ComposerObjectTypes::Enum type,
                                                     TInstanceHandle parent, TSlideHandle inSlide,
                                                     DocumentEditorInsertType::Enum inInsertType,
                                                     const CPt &inPosition,
                                                     EPrimitiveType inPrimitiveType,
                                                     long inStartTime) = 0;

    virtual void DeleteInstances(qt3dsdm::TInstanceHandleList inInstances) = 0;
    // Delete this data model instance.  Will recursively delete any attached children in the scene
    // graph
    // if this instance is represented in the scene graph.
    void DeleteInstance(TInstanceHandle instance)
    {
        qt3dsdm::TInstanceHandleList theInstances;
        theInstances.push_back(instance);
        DeleteInstances(theInstances);
    }

    // Set the instance's property value.  This will set it in this slide if specified, else it is
    // set
    // in the datacore.  Mainly for internal use.  Most clients should use the API's below (note
    // that lack of slide information
    virtual void SetSpecificInstancePropertyValue(TSlideHandle inSlide, TInstanceHandle instance,
                                                  TPropertyHandle inProperty,
                                                  const SValue &value) = 0;
    void SetSpecificInstancePropertyValue(TSlideHandle inSlide, TInstanceHandle instance,
                                          const wchar_t *inProperty, const SValue &value)
    {
        SetSpecificInstancePropertyValue(inSlide, instance, FindProperty(instance, inProperty),
                                         value);
    }

    // Set the property going through the property system and without a property lookup.
    // autoDelete tells this object to do things like:
    // delete the image that was there if the GUID is different.
    // Delete the group subtree if the user imports a different path over the group.
    virtual void SetInstancePropertyValue(TInstanceHandle instance, TPropertyHandle propName,
                                          const SValue &value, bool autoDelete = true) = 0;

    // Create an image, generate the GUID, and go from there.
    // Returns an invalid instance in the case of failure.
    // inSourcePath is assumed to !!already be document relative!!
    // CDoc::GetRelativePathToDoc
    // In fact, if ResolvePathToDoc doesn't return a valid file,
    // the we will return 0 from this function.
    virtual TInstanceHandle
    SetInstancePropertyValueAsImage(TInstanceHandle instance, TPropertyHandle propName,
                                    const Q3DStudio::CString &inSourcePath) = 0;

    // If sourcepath resolves to a valid plugin file, we create a renderable item and add it as a
    // child to the image.
    // else we return nothing and just set the value as is.
    virtual TInstanceHandle
    SetInstancePropertyValueAsRenderable(TInstanceHandle instance, TPropertyHandle propName,
                                         const Q3DStudio::CString &inSourcePath) = 0;

    virtual void SetMaterialType(TInstanceHandle instance,
                                 const Q3DStudio::CString &inRelativePathToMaterialFile) = 0;

    // Set the slide name property value
    // Also update all actions that point to the old slide name to new name
    virtual void SetSlideName(TInstanceHandle instance, TPropertyHandle propName,
                              const wchar_t *inOldName, const wchar_t *inNewName) = 0;

    virtual void SetName(Qt3DSDMInstanceHandle inInstance, const CString &inName,
                         bool inMakeUnique = false) = 0;

    // Linking/unlinking properties.  Functions take care of creating/destroying
    // images where necessary.
    virtual void UnlinkProperty(TInstanceHandle instance, TPropertyHandle propName) = 0;
    virtual void LinkProperty(TInstanceHandle instance, TPropertyHandle propName) = 0;

    virtual void SetTimeRange(TInstanceHandle inInstance, long inStart, long inEnd) = 0;
    virtual void SetTimeRangeInSlide(TSlideHandle inSlide, TInstanceHandle inInstance, long inStart,
                                     long inEnd) = 0;
    virtual void SetStartTime(TInstanceHandle inInstance, long inStart) = 0;
    virtual void SetEndTime(TInstanceHandle inInstance, long inEnd) = 0;
    virtual void ResizeTimeRange(TInstanceHandle inInstance, long inTime, bool inSetStart) = 0;
    virtual void OffsetTimeRange(TInstanceHandle inInstance, long inOffset) = 0;
    // called when the user hits '[' or ']'.
    virtual void TruncateTimeRange(TInstanceHandle inInstance, bool inSetStart, long inTime) = 0;

    virtual void SetTimebarColor(TInstanceHandle inInstance, ::CColor inColor) = 0;
    virtual void SetTimebarText(TInstanceHandle inInstance,
                                const Q3DStudio::CString &inComment) = 0;

    // Scene graph management.
    virtual void AddChild(TInstanceHandle parent, TInstanceHandle child,
                          TInstanceHandle inNextSibling = TInstanceHandle()) = 0;
    virtual void RemoveChild(TInstanceHandle parent, TInstanceHandle child) = 0;

    // Set animation values.  Animations that aren't considered user edited
    // are replaced during refresh operations.
    // CreateOrSetAnimation will offset each keyframe by the object's start time; thus the keyframe
    // time space is expected to be normalized to the object's start time when being input to this
    // function.
    virtual Qt3DSDMAnimationHandle
    CreateOrSetAnimation(TSlideHandle inSlide, TInstanceHandle instance, const wchar_t *propName,
                         long subIndex, qt3dsdm::EAnimationType animType, const float *keyframeValues,
                         long numValues, bool inUserEdited = true) = 0;
    virtual bool RemoveAnimation(TSlideHandle inSlide, TInstanceHandle instance,
                                 const wchar_t *propName, long subIndex) = 0;
    virtual void SetKeyframeTime(TKeyframeHandle inKeyframe, long inTimeInMilliseconds) = 0;
    virtual void DeleteAllKeyframes(Qt3DSDMAnimationHandle inAnimation) = 0;
    virtual void KeyframeProperty(Qt3DSDMInstanceHandle inInstance, Qt3DSDMPropertyHandle inProperty,
                                  bool inDoDiffValue) = 0;

    // Only the import interface needs to worry about this.  The animation system automatically
    // sets the flag upon any change to an animation.
    virtual void SetIsArtistEdited(Qt3DSDMAnimationHandle inAnimation, bool inEdited = true) = 0;

    // Paste a scene graph object into this system at this location.  Returns the new object
    //(but it sets the new object as the selected object so clients probably don't need this)
    virtual qt3dsdm::TInstanceHandleList
    PasteSceneGraphObject(const CFilePath &inFilePath, Qt3DSDMInstanceHandle inNewRoot,
                          bool inGenerateUniqueName, DocumentEditorInsertType::Enum inInsertType,
                          const CPt &inPosition) = 0;

    virtual qt3dsdm::TInstanceHandleList PasteSceneGraphObjectMaster(
        const CFilePath &inFilePath, Qt3DSDMInstanceHandle inNewRoot, bool inGenerateUniqueName,
        DocumentEditorInsertType::Enum inInsertType, const CPt &inPosition) = 0;

    virtual void RearrangeObjects(const qt3dsdm::TInstanceHandleList &inInstances,
                                  TInstanceHandle inDest,
                                  DocumentEditorInsertType::Enum inInsertType) = 0;

    void RearrangeObject(TInstanceHandle inInstance, TInstanceHandle inDest,
                         DocumentEditorInsertType::Enum inInsertType)
    {
        qt3dsdm::TInstanceHandleList theInstances;
        theInstances.push_back(inInstance);
        RearrangeObjects(theInstances, inDest, inInsertType);
    }

    virtual void ReorderRows(qt3dsdm::Qt3DSDMInstanceHandle handleSource,
                             qt3dsdm::Qt3DSDMInstanceHandle handleParent, int index) = 0;

    // Returns the new component.
    virtual TInstanceHandle MakeComponent(const qt3dsdm::TInstanceHandleList &inInstances) = 0;

    virtual void CreateAliasDuplicates(const qt3dsdm::TInstanceHandleList &inInstances,
                                       qt3dsdm::Qt3DSDMSlideHandle theSlide) = 0;

    virtual qt3dsdm::TInstanceHandleList
    DuplicateInstances(const qt3dsdm::TInstanceHandleList &inInstances, TInstanceHandle inDest,
                       DocumentEditorInsertType::Enum inInsertType) = 0;

    TInstanceHandle DuplicateInstance(TInstanceHandle inInstance, TInstanceHandle inDest,
                                      DocumentEditorInsertType::Enum inInsertType)
    {
        qt3dsdm::TInstanceHandleList theInstances;
        theInstances.push_back(inInstance);
        theInstances = DuplicateInstances(theInstances, inDest, inInsertType);
        if (theInstances.empty() == false)
            return theInstances.back();
        return TInstanceHandle();
    }

    virtual void DuplicateInstances(const qt3dsdm::TInstanceHandleList &inInstances) = 0;
    void DuplicateInstance(TInstanceHandle inInstance)
    {
        qt3dsdm::TInstanceHandleList theInstances;
        theInstances.push_back(inInstance);
        return DuplicateInstances(theInstances);
    }

    virtual Qt3DSDMActionHandle AddAction(Qt3DSDMSlideHandle inSlide, Qt3DSDMInstanceHandle inOwner,
                                         const wstring &inEvent, const wstring &inHandler) = 0;

    virtual void DeleteAction(Qt3DSDMActionHandle inAction) = 0;

    // Paste a given action into this new root object.
    virtual Qt3DSDMActionHandle PasteAction(const CFilePath &inFilePath,
                                           Qt3DSDMInstanceHandle inNewRoot) = 0;

    virtual Qt3DSDMSlideHandle AddSlide(Qt3DSDMSlideHandle inMasterSlide, int inIndex = -1) = 0;

    // Only valid if the master slide has more than one slide.
    virtual void DeleteSlide(Qt3DSDMSlideHandle inSlide) = 0;

    virtual void RearrangeSlide(Qt3DSDMSlideHandle inSlide, int inNewIndex) = 0;

    virtual Qt3DSDMSlideHandle DuplicateSlide(Qt3DSDMSlideHandle inSlide) = 0;

    virtual qt3dsdm::Qt3DSDMGuideHandle CreateGuide(const qt3dsdm::SGuideInfo &inInfo) = 0;
    virtual void UpdateGuide(qt3dsdm::Qt3DSDMGuideHandle hdl, const qt3dsdm::SGuideInfo &inInfo) = 0;
    virtual void DeleteGuide(qt3dsdm::Qt3DSDMGuideHandle hdl) = 0;
    virtual void ClearGuides() = 0;

    // Imports a colladafile and, if successful, returns the instance handle from the imported
    // document.
    // This function takes care of throwing up dialogues and such if the import fails.
    // Returns an invalid handle (0) if the import fails.
    // This is the only true import operation.  The rest don't move the files into the
    // document at this point.
    virtual TInstanceHandle ImportDAE(const Q3DStudio::CString &inFullPathToDocument,
                                      TInstanceHandle inParent, TSlideHandle inSlide,
                                      const Q3DStudio::CString &inImportFileExtension,
                                      DocumentEditorInsertType::Enum inDropType,
                                      const CPt &inPosition = CPt(), long inStartTime = -1) = 0;

    virtual TInstanceHandle LoadImportFile(const Q3DStudio::CString &inFullPathToDocument,
                                           TInstanceHandle inParent, TSlideHandle inSlide,
                                           DocumentEditorInsertType::Enum inDropType,
                                           const CPt &inPosition = CPt(),
                                           long inStartTime = -1) = 0;
    // Automap an image into the scene
    virtual TInstanceHandle AutomapImage(const Q3DStudio::CString &inFullPathToDocument,
                                         TInstanceHandle inParent, TSlideHandle inSlide,
                                         DocumentEditorInsertType::Enum inDropType,
                                         const CPt &inPosition = CPt(), long inStartTime = -1) = 0;

    virtual TInstanceHandle LoadMesh(const Q3DStudio::CString &inFullPathToDocument,
                                     TInstanceHandle inParent, TSlideHandle inSlide,
                                     DocumentEditorInsertType::Enum inDropType,
                                     const CPt &inPosition = CPt(), long inStartTime = -1) = 0;

    virtual TInstanceHandle LoadBehavior(const Q3DStudio::CString &inFullPathToDocument,
                                         TInstanceHandle inParent, TSlideHandle inSlide,
                                         DocumentEditorInsertType::Enum inDropType,
                                         long inStartTime) = 0;

    virtual TInstanceHandle LoadRenderPlugin(const Q3DStudio::CString &inFullPathToDocument,
                                             TInstanceHandle inParent, TSlideHandle inSlide,
                                             DocumentEditorInsertType::Enum inDropType,
                                             long inStartTime) = 0;

    virtual TInstanceHandle LoadCustomMaterial(const Q3DStudio::CString &inFullPathToDocument,
                                               TInstanceHandle inParent, TSlideHandle inSlide,
                                               DocumentEditorInsertType::Enum inDropType,
                                               long inStartTime,
                                               TInstanceHandle inTargetId = TInstanceHandle()) = 0;

    // Create text from the font file
    virtual TInstanceHandle CreateText(const Q3DStudio::CString &inFullPathToDocument,
                                       TInstanceHandle inParent, TSlideHandle inSlide,
                                       DocumentEditorInsertType::Enum inDropType,
                                       const CPt &inPosition = CPt(), long inStartTime = -1) = 0;

    virtual TInstanceHandle LoadEffect(const Q3DStudio::CString &inFullPathToDocument,
                                       TInstanceHandle inParent, TSlideHandle inSlide,
                                       DocumentEditorInsertType::Enum inDropType,
                                       long inStartTime) = 0;

    virtual TInstanceHandle
    ImportFile(DocumentEditorFileType::Enum inFileType,
               const Q3DStudio::CString &inFullPathToDocument, TInstanceHandle inParent,
               TSlideHandle inSlide, const Q3DStudio::CString &inImportFileExtension,
               DocumentEditorInsertType::Enum inDropType = DocumentEditorInsertType::LastChild,
               const CPt &inPosition = CPt(), long inStartTime = -1) = 0;

    // Refresh an import or dae file
    // Absolute path to the file.
    virtual void RefreshImport(const CFilePath &inOldFile, const CFilePath &inNewFile) = 0;

    virtual void ReplaceTextFontNameWithTextFileStem(qt3ds::render::ITextRenderer &inRenderer) = 0;

    virtual void ExternalizePath(TInstanceHandle path) = 0;
    virtual void InternalizePath(TInstanceHandle path) = 0;

    static std::shared_ptr<IDOMReader>
    ParseScriptFile(const Q3DStudio::CFilePath &inFullPathToDocument,
                    std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                    std::shared_ptr<IImportFailedHandler> inHandler,
                    qt3ds::render::IInputStreamFactory &inInputStreamFactory);

    static std::shared_ptr<IDOMReader>
    ParsePluginFile(const Q3DStudio::CFilePath &inFullPathToDocument,
                    std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                    std::shared_ptr<IImportFailedHandler> inHandler,
                    qt3ds::render::IInputStreamFactory &inInputStreamFactory);

    static std::shared_ptr<IDOMReader>
    ParseCustomMaterialFile(const Q3DStudio::CFilePath &inFullPathToDocument,
                            std::shared_ptr<qt3dsdm::IStringTable> inStringTable,
                            std::shared_ptr<IImportFailedHandler> inHandler,
                            qt3ds::render::IInputStreamFactory &inInputStreamFactory);

    static void DisplayImportErrors(const QString &inImportSource,
                                    qt3dsimp::ImportErrorCodes::Enum inImportError,
                                    std::shared_ptr<IImportFailedHandler> inHandler,
                                    qt3dsimp::STranslationLog &inLog, bool inForceError = false);
};

struct ScopedDocumentEditor
{
    IDocumentEditor &m_Editor;
    ScopedDocumentEditor(IDoc &inDoc, const Q3DStudio::CString &inCommandName, const char *inFile,
                         int inLine);
    ~ScopedDocumentEditor() { m_Editor.Release(); }
    IDocumentEditor *operator->() { return &m_Editor; }
};

#define SCOPED_DOCUMENT_EDITOR(doc, cmdname) ScopedDocumentEditor(doc, Q3DStudio::CString::fromQString(cmdname), __FILE__, __LINE__)

class CUpdateableDocumentEditor
{
    IDoc &m_EditorIDocDoc;
    const char *m_File;
    int m_Line;

    CUpdateableDocumentEditor(const CUpdateableDocumentEditor &other);
    CUpdateableDocumentEditor &operator=(const CUpdateableDocumentEditor &other);

public:
    CUpdateableDocumentEditor(IDoc &inDoc)
        : m_EditorIDocDoc(inDoc)
        , m_File(NULL)
    {
    }
    ~CUpdateableDocumentEditor();
    IDoc &GetEditorDoc() { return m_EditorIDocDoc; }
    bool HasEditor() const;
    IDocumentEditor &EnsureEditor(const wchar_t *inCommandName, const char *inFile, int inLine);
    void FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle *inInstances, long inInstanceCount);
    void FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    {
        FireImmediateRefresh(&inInstance, 1);
    }
    void CommitEditor();
    void RollbackEditor();
};

#define ENSURE_EDITOR(cmdName) EnsureEditor(cmdName, __FILE__, __LINE__)

class IInternalDocumentEditor : public IDocumentEditor
{
public:
    virtual ~IInternalDocumentEditor() {}
    static std::shared_ptr<IInternalDocumentEditor> CreateEditor(CDoc &doc);
};
};

#endif
