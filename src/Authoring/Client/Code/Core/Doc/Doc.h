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
#ifndef INCLUDED_DOC_H
#define INCLUDED_DOC_H 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Conditional.h"
#include "UICFile.h"
#include "UICRect.h"
#include "IDoc.h"
#include "GUIDUtilities.h"
#include "StudioErrorIDs.h"
#include "GraphUtils.h"
#include "CmdStackModifier.h"
#include "UICColor.h"
#include "SelectedValueImpl.h"
#include "Utility/CoreConst.h"
#include <quuid.h>

//==============================================================================
//	Forwards
//==============================================================================
class CScene;
class CCamera;
class CLight;
class CHotKeys;
class CCmdBatch;
class CStudioProjectSettings;
class CView;
class ISelectable;
class CBufferedInputStream;
class CBufferedOutputStream;
class CDataManager;
class CClientDataModelBridge;
class IUICDMSerializer;
class IKeyframesManager;
class IObjectReferenceHelper;
class CCore;
class CRenderContext;
class CPlaybackClock;

namespace Q3DStudio {
class IInternalDocumentEditor;
class CFilePath;
class IInternalDirectoryWatchingSystem;
class IDirectoryWatchingSystem;
class IImportFailedHandler;
class IDeletingReferencedObjectHandler;
class CTransactionCloseListenerSignaller;
class IDocSceneGraph;
struct SSelectedValue;
}

namespace UICDM {
class CStudioSystem;
class ISignalConnection;
class CmdDataModel;
}

struct SDocumentDataModelListener;

namespace std {
template <>
struct hash<GUID>
{
    size_t operator()(const GUID &guid) const
    {
        QUuid quuid;
        quuid.data1 = guid.Data1;
        quuid.data2 = guid.Data2;
        quuid.data3 = guid.Data3;
        memcpy(quuid.data4, guid.Data4, 8);
        return qHash(quuid);
    }
};
struct AreGuidsEqual
{

    bool operator()(const GUID &lhs, const GUID &rhs) const
    {
        return memcmp(&lhs, &rhs, sizeof(GUID)) == 0;
    }
};
}


struct SubPresentationRecord
{
    QString m_type;
    QString m_id;
    QString m_argsOrSrc;

    SubPresentationRecord() {}
    SubPresentationRecord(const QString &type, const QString &id, const QString &args)
        : m_type(type), m_id(id), m_argsOrSrc(args)
    {

    }

    SubPresentationRecord &operator = (const SubPresentationRecord& o)
    {
        m_type = o.m_type;
        m_id = o.m_id;
        m_argsOrSrc = o.m_argsOrSrc;
        return *this;
    }
};

//==============================================================================
//	CDoc Class
//==============================================================================

class CDoc : public IDoc, public ICmdStackModifier
{

public:
    friend struct SDocumentDataModelListener;
    CDoc(CCore *inCore);
    virtual ~CDoc();

    DEFINE_OBJECT_COUNTER(CDoc)

    void
    SetDirectoryWatchingSystem(std::shared_ptr<Q3DStudio::IDirectoryWatchingSystem> inSystem);
    void SetImportFailedHandler(std::shared_ptr<Q3DStudio::IImportFailedHandler> inHandler);
    std::shared_ptr<Q3DStudio::IImportFailedHandler> GetImportFailedHandler();
    void SetDocMessageBoxHandler(
        std::shared_ptr<Q3DStudio::IDeletingReferencedObjectHandler> inHandler);

    // The system may be null in the case where we are running without a UI.
    Q3DStudio::IDirectoryWatchingSystem *GetDirectoryWatchingSystem();
    void SetDocumentPath(const CUICFile &inFile);
    CUICFile GetDocumentPath() const;
    bool IsInDocSubDirectory(const Q3DStudio::CString &inPath) const;
    Q3DStudio::CString GetDocumentDirectory() const;
    Q3DStudio::CString GetRelativePathToDoc(const Q3DStudio::CFilePath &inPath);
    Q3DStudio::CString GetResolvedPathToDoc(const Q3DStudio::CFilePath &inPath);

    CUICFile CreateUntitledDocument() const;

    void CloseDocument();
    void LoadDocument(const CUICFile &inDocument);
    void SaveDocument(const CUICFile &inDocument);
    void CreateNewDocument();
    QString GetDocumentUIAFile();
    void LoadUIASubpresentations(const QString &uiaFile,
                                 QVector<SubPresentationRecord> &subpresentations);

    bool IsModified();
    bool IsValid() const;

    UICDM::CUICDMInstanceHandle GetInstanceFromSelectable(Q3DStudio::SSelectedValue inSelectedItem);
    UICDM::CUICDMInstanceHandle GetSelectedInstance();

    void CutSelectedObject();
    void DeleteSelectedObject();
    bool DeleteSelectedKeys();
    void SetChangedKeyframes();

    // Cut object to clipboard
    void CutObject(UICDM::TInstanceHandleList inInstance);
    void CutObject(UICDM::CUICDMInstanceHandle inInstance)
    {
        UICDM::TInstanceHandleList objects;
        objects.push_back(inInstance);
        CutObject(objects);
    }
    // copy object to clipboard
    void CopyObject(UICDM::TInstanceHandleList inInstance);
    void CopyObject(UICDM::CUICDMInstanceHandle inInstance)
    {
        UICDM::TInstanceHandleList objects;
        objects.push_back(inInstance);
        CopyObject(objects);
    }
    // paste object to clipboard
    void PasteObject(UICDM::CUICDMInstanceHandle inInstance);
    void PasteObjectMaster(UICDM::CUICDMInstanceHandle inInstance);
    void DeleteObject(const UICDM::TInstanceHandleList &inInstances);

    void DeleteObject(UICDM::CUICDMInstanceHandle inInstance)
    {
        UICDM::TInstanceHandleList theInstances;
        theInstances.push_back(inInstance);
        DeleteObject(theInstances);
    }

    void ClientStep();

    void HandleCopy();
    void HandlePaste();
    void HandleMasterPaste();
    void HandleCut();

    bool CanCopyObject(); // specifically for objects
    bool CanCopyObject(const UICDM::TInstanceHandleList &inInstances); // specifically for objects
    bool CanCopyKeyframe(); // specifically for keyframes
    bool CanCopyAction(); // specifically for actions
    bool CanPasteObject(); // specifically for objects
    bool CanPasteKeyframe(); // specifically for keyframes
    bool CanPasteAction(); // specifically for actions

    bool CanPaste(); // for objects or keyframes or actions
    bool CanCopy(); // for objects or keyframes or actions
    bool CanCut(); // for objects or keyframes or actions
    void HandleDuplicateCommand();

    bool VerifyCanRename(UICDM::CUICDMInstanceHandle inAsset);

    void DeselectAllItems(bool inSendEvent);
    void DeselectAllItems() { DeselectAllItems(true); }

    UICDM::CUICDMInstanceHandle GetActiveLayer();
    void SetActiveLayer(UICDM::CUICDMInstanceHandle inLayerInstance);
    UICDM::CUICDMSlideHandle GetActiveSlide();
    void OnLayerDeleted(UICDM::CUICDMInstanceHandle inLayerInstance);

    void SetPlayMode(EPlayMode inPlayMode, long inRestoreTime = -1);
    bool IsPlaying();
    long GetCurrentClientTime();

    void RegisterGlobalKeyboardShortcuts(CHotKeys *inShortcutHandler);

    UICDM::CUICDMInstanceHandle GetSceneInstance() { return m_SceneInstance; }

    // IDoc
    virtual UICDM::CUICDMInstanceHandle GetActiveRootInstance();
    long GetCurrentViewTime() const override;
    virtual void OnComponentSeconds();
    // Notify time changed and set the playback clock to this time.
    void NotifyTimeChanged(long inNewTime) override;
    // Notify time changed.
    virtual void DoNotifyTimeChanged(long inNewTime);
    void NotifyActiveSlideChanged(UICDM::CUICDMSlideHandle inNewActiveSlide) override;
    void NotifyActiveSlideChanged(UICDM::CUICDMSlideHandle inNewActiveSlide,
                                          bool inForceRefresh,
                                          bool inIgnoreLastDisplayTime = false) override;
    virtual void
    NotifySelectionChanged(Q3DStudio::SSelectedValue inNewSelection = Q3DStudio::SSelectedValue()) override;
    virtual Q3DStudio::SSelectedValue GetSelectedValue() const { return m_SelectedValue; }
    void SetKeyframeInterpolation() override;
    void DeselectAllKeyframes() override;
    void SetModifiedFlag(bool inIsModified = true) override;
    void SetKeyframesManager(IKeyframesManager *inManager) override;
    IKeyframesManager *GetKeyframesManager() override;
    UICDM::IPropertySystem *GetPropertySystem() override;
    UICDM::IAnimationCore *GetAnimationCore() override;
    void SetInstancePropertyValue(UICDM::CUICDMInstanceHandle inInstance,
                                          const std::wstring &inPropertyName,
                                          const UICDM::SValue &inValue) override;
    Q3DStudio::IDocumentBufferCache &GetBufferCache() override;
    Q3DStudio::IDocumentReader &GetDocumentReader() override;
    Q3DStudio::IDocumentEditor &OpenTransaction(const Q3DStudio::CString &inCmdName,
                                                        const char *inFile, int inLine) override;
    Q3DStudio::IDocumentEditor &MaybeOpenTransaction(const Q3DStudio::CString &cmdName,
                                                             const char *inFile, int inLine) override;
    bool IsTransactionOpened() const override;
    void RollbackTransaction() override;
    void CloseTransaction() override;
    void IKnowWhatIAmDoingForceCloseTransaction() override;

    std::shared_ptr<Q3DStudio::IComposerSerializer> CreateSerializer() override;
    virtual std::shared_ptr<Q3DStudio::IComposerSerializer> CreateTransactionlessSerializer();
    // Create a DOM writer that is opened to the project element.  This is where the serializer
    // should write to.
    std::shared_ptr<UICDM::IDOMWriter> CreateDOMWriter() override;
    // Create a DOM reader and check that the top element's version is correct.  Opens the reader
    // to the project element.
    virtual std::shared_ptr<UICDM::IDOMReader>
    CreateDOMReader(const Q3DStudio::CString &inFilePath, qt3ds::QT3DSI32 &outVersion) override;
    virtual std::shared_ptr<UICDM::IDOMReader> CreateDOMReader(CBufferedInputStream &inStream,
                                                                 qt3ds::QT3DSI32 &outVersion);

    void SelectUICDMObject(UICDM::CUICDMInstanceHandle inInstanceHandle);
    // multiselect support
    void ToggleUICDMObjectToSelection(UICDM::CUICDMInstanceHandle inInstanceHandle);
    void SelectAndNavigateToUICDMObject(UICDM::CUICDMInstanceHandle inInstanceHandle);
    long GetLatestEndTime();

    CCore *GetCore() override;

    void TruncateTimebar(bool inSetStart, bool inAffectsChildren);

    // Helper methods for Edit Camera
    // These need to move to the render system.
    // They are view specific properties and have nothing to do with
    // the document.
    /*
    CEditCameraContainer*       GetEditCameraContainer( );
    CCamera*                    GetCurrentEditCamera( );
    CLight*                     GetEditingLight( );
    long                        GetEditingFillMode( );
    bool                        IsInEditMode( );
    void                        SetEditViewBackgroundColor( ::CColor& inColor
    );
    Q3DStudio::CColor           GetEditViewBackgroundColor( );
    */

    UICDM::CStudioSystem *GetStudioSystem() override { return m_StudioSystem.get(); }

    IObjectReferenceHelper *GetDataModelObjectReferenceHelper() const
    {
        return m_DataModelObjectRefHelper;
    }

    void SetDefaultKeyframeInterpolation(bool inSmooth);
    void ScheduleRemoveImageInstances(UICDM::CUICDMInstanceHandle inInstance, CCmdBatch *outBatch);
    void ScheduleRemoveDataModelInstances(UICDM::CUICDMInstanceHandle inInstance,
                                          CCmdBatch *outBatch);
    void ScheduleRemoveComponentInstances(UICDM::CUICDMInstanceHandle inInstance,
                                          CCmdBatch *outBatch);

    TAssetGraphPtr GetAssetGraph() { return m_AssetGraph; }
    virtual void AddToGraph(UICDM::CUICDMInstanceHandle inParentInstance,
                            UICDM::CUICDMInstanceHandle inInstance);

    // helper
    void IterateImageInstances(UICDM::CUICDMInstanceHandle inInstance,
                               std::vector<Q3DStudio::CId> *outImageIdList);
    UICDM::CUICDMInstanceHandle GetObjectbySelectMode(UICDM::CUICDMInstanceHandle inInstance,
                                                      bool inGroupMode);

    // ICmdStackModifier
    bool CanUndo() override;
    bool PreUndo() override;

    void CheckActionDependencies(UICDM::CUICDMInstanceHandle inInstance);
    void SetActiveSlideWithTransaction(UICDM::CUICDMSlideHandle inNewActiveSlide);

    void SetSceneGraph(std::shared_ptr<Q3DStudio::IDocSceneGraph> inGraph);
    Q3DStudio::IDocSceneGraph *GetSceneGraph() { return m_SceneGraph.get(); }

    void GetProjectFonts(
        std::vector<std::pair<Q3DStudio::CString, Q3DStudio::CString>> &outFontNameFileList);
    void GetProjectFonts(std::vector<Q3DStudio::CString> &outFonts);
    Q3DStudio::CString
    GetProjectFontName(const Q3DStudio::CFilePath
                           &inFullPathToFontFile); // Given a font file, return the font name

protected:
    // Set the active slide, return true if delving
    void SetActiveSlideChange(UICDM::CUICDMSlideHandle inNewActiveSlide);
    void OnSlideDeleted(UICDM::CUICDMSlideHandle inSlide);
    void OnInstanceDeleted(UICDM::CUICDMInstanceHandle inInstance);
    Q3DStudio::SSelectedValue SetupInstanceSelection(UICDM::CUICDMInstanceHandle inInstance);
    // Set the selection, but don't send an event.
    bool SetSelection(Q3DStudio::SSelectedValue inNewSelection = Q3DStudio::SSelectedValue());
    void LoadPresentationFile(CBufferedInputStream *inInputStream);
    void SavePresentationFile(CBufferedOutputStream *inOutputStream);

    void CleanupData();
    void ResetData();
    void LoadStudioData(CBufferedInputStream *inInputStream);
    void ResetDataCore();
    void SetupDataCoreSignals();

    void CreatePresentation();
    void ClosePresentation();

    void DeleteSelectedItems();
    void GetActionDependencies(UICDM::CUICDMInstanceHandle inInstance,
                               Q3DStudio::CString &ioActionDependencies);
    void GetActionDependencies(UICDM::CUICDMInstanceHandle inInstance,
                               UICDM::TActionHandleList &ioActionList);

    bool OnNudgeKey(int inChar, int inRepeatCount, Qt::KeyboardModifiers modifiers);
    void OnNudgeDone();

    UICDM::CUICDMInstanceHandle GetFirstSelectableLayer();
    UICDM::CUICDMInstanceHandle GetTopmostGroup(UICDM::CUICDMInstanceHandle inInstance);

    void GetActionsAffectedByRename(UICDM::CUICDMInstanceHandle inAsset,
                                    std::set<Q3DStudio::CString> &ioActionsAffected);

    bool isFocusOnTextEditControl();

    //==========================================================================
    //	Protected Fields
    //==========================================================================

    long m_PlayMode; ///< This tracks whether we're playing a client presentation or not.
    Q3DStudio::SSelectedValue m_SelectedObject; ///< DIE USELESS COMMENTS DIE.
    long m_CurrentViewTime; ///< The current time that is displayed by the playhead, not necessarily
                            ///the client time.
    UICDM::CUICDMInstanceHandle m_SceneInstance; ///< Pointer to the root level Scene object.
    UICDM::CUICDMSlideHandle m_ActiveSlide; ///< The currently active Slide Handle.
    UICDM::CUICDMInstanceHandle m_ActiveLayer; ///< The currently active layer.
    CPlaybackClock *m_PlaybackClock; ///< Playback clock. This is used when user clicks "Play"
    CCore *m_Core;
    bool m_IsModified;
    bool m_IsTemporary;
    CUICFile m_DocumentPath;

    CDataManager *m_DataManager; ///< Manager for handling data properties.

    std::shared_ptr<UICDM::CStudioSystem> m_StudioSystem;

    IKeyframesManager *m_KeyframesManager; ///< To support menu actions for selected keys

    IObjectReferenceHelper *m_DataModelObjectRefHelper; ///< To support object reference control

    TAssetGraphPtr m_AssetGraph;

    std::shared_ptr<Q3DStudio::IInternalDocumentEditor> m_SceneEditor;
    std::shared_ptr<SDocumentDataModelListener> m_DataModelListener;
    std::shared_ptr<Q3DStudio::IDocumentBufferCache> m_DocumentBufferCache;
    std::vector<std::shared_ptr<UICDM::ISignalConnection>> m_Connections;
    std::shared_ptr<Q3DStudio::IDirectoryWatchingSystem> m_DirectoryWatchingSystem;
    std::shared_ptr<Q3DStudio::IImportFailedHandler> m_ImportFailedHandler;
    std::shared_ptr<Q3DStudio::IDeletingReferencedObjectHandler>
        m_DeletingReferencedObjectHandler;
    long m_TransactionDepth;
    std::shared_ptr<UICDM::CmdDataModel> m_OpenTransaction;
    std::shared_ptr<Q3DStudio::IDocSceneGraph> m_SceneGraph;
    Q3DStudio::SSelectedValue m_SelectedValue;

    void GetOrUpdateFileList(std::vector<Q3DStudio::CString> &ioMyList,
                             std::vector<Q3DStudio::CString> &outResult,
                             const wchar_t **inExtensionList) const;

public:
    void OnNewPresentation();
    void OnPresentationDeactivated();

protected:
    CRenderContext *m_RenderContext; ///< The render context attached to this player's window.
    UICRenderDevice m_WindowHandle; ///< The window handle to which to render
    Q3DStudio::CRect m_ClientSize;
    Q3DStudio::CRect m_SceneRect; ///< The dimensions of the active scene view
};

#endif // INCLUDED_DOC_H
