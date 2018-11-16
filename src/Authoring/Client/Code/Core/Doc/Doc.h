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
#include "Qt3DSFile.h"
#include "Qt3DSRect.h"
#include "IDoc.h"
#include "GUIDUtilities.h"
#include "GraphUtils.h"
#include "CmdStackModifier.h"
#include "SelectedValueImpl.h"
#include "Utility/CoreConst.h"

#include <QtCore/quuid.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qsize.h>

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
class IMoveRenameHandler;
class CTransactionCloseListenerSignaller;
class IDocSceneGraph;
struct SSelectedValue;
}

namespace qt3dsdm {
class CStudioSystem;
class ISignalConnection;
class CmdDataModel;
}

QT_FORWARD_DECLARE_CLASS(QWidget)

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
    QSize m_size;

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
        m_size = o.m_size;
        return *this;
    }

    bool operator == (const SubPresentationRecord &r) const
    {
        return r.m_id == m_id && r.m_argsOrSrc == m_argsOrSrc && r.m_type == m_type;
    }

    friend QDebug operator << (QDebug debug, const SubPresentationRecord &rec);
};

class CDataInputDialogItem
{
public:
    QString valueString;
    float minValue;
    float maxValue;
    QString name;
    int type;
    QVector<qt3dsdm::Qt3DSDMInstanceHandle> controlledElems;
    // The type of property/properties that this datainput controls
    // for the purposes of limiting allowed datainput type changes.
    // Note that there can be more than one type being controlled
    // f.ex with Variant type. Boolean signifies "strict" type requirement
    // i.e. only the exact equivalent mapping from datainput type to
    // property type is allowed (float to float, string to string etc.)
    QVector<QPair<qt3dsdm::DataModelDataType::Value, bool>> boundTypes;
    // Bindings in other subpresentations, of QMap format
    // QMultiMap<subpresentation_id, QPair<datatype, strict>>.
    // Stored separately so we can conveniently update/clear binding info
    // for current presentation and keep subpresentation binding info intact or vice versa.
    QMultiMap<QString, QPair<qt3dsdm::DataModelDataType::Value, bool>> externalPresBoundTypes;
};

//==============================================================================
//	CDoc Class
//==============================================================================

class CDoc : public QObject, public IDoc, public ICmdStackModifier
{
    Q_OBJECT
public:
    friend struct SDocumentDataModelListener;
    CDoc(CCore *inCore);
    virtual ~CDoc() override;

    DEFINE_OBJECT_COUNTER(CDoc)

    void
    SetDirectoryWatchingSystem(std::shared_ptr<Q3DStudio::IDirectoryWatchingSystem> inSystem);
    void SetImportFailedHandler(std::shared_ptr<Q3DStudio::IImportFailedHandler> inHandler);
    std::shared_ptr<Q3DStudio::IImportFailedHandler> GetImportFailedHandler();
    void SetDocMessageBoxHandler(
            std::shared_ptr<Q3DStudio::IDeletingReferencedObjectHandler> inHandler);
    void setMoveRenameHandler(std::shared_ptr<Q3DStudio::IMoveRenameHandler> inHandler);
    std::shared_ptr<Q3DStudio::IMoveRenameHandler> getMoveRenameHandler();

    // The system may be null in the case where we are running without a UI.
    Q3DStudio::IDirectoryWatchingSystem *GetDirectoryWatchingSystem();
    bool SetDocumentPath(const QString &inFile);
    QString GetDocumentPath() const;
    void setPresentationId(const QString &id);
    QString getPresentationId() const;
    QString GetDocumentDirectory() const;
    QString getRelativePath() const;
    QString GetResolvedPathToDoc(const QFileInfo &inPath);
    QString GetRelativePathToDoc(const QFileInfo &inPath);

    QString CreateUntitledDocument() const;

    void CloseDocument();
    void LoadDocument(const QString &inDocument);
    void SaveDocument(const QString &inDocument);
    void CreateNewDocument();
    // In outMap, returns datainput names found from element control
    // bindings but which are missing from (UIP) datainput list
    void UpdateDatainputMap(
            const qt3dsdm::Qt3DSDMInstanceHandle inInstance,
            QMultiMap<QString,
                      QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                            qt3dsdm::Qt3DSDMPropertyHandle>> *outMap = nullptr);
    // Sanity checks controlledproperty strings to see if controller names
    // and target properties are valid. Removes invalid controller - property
    // pairs from controlledproperty string. Transaction/undo points are not created
    // for invalid pair deletions, so caller is responsible for dispatching datamodel
    // notifications.
    // Recurses through the entire tree and returns true only if all strings were valid.
    bool VerifyControlledProperties(const qt3dsdm::Qt3DSDMInstanceHandle inInstance);

    bool IsModified();
    bool IsValid() const;

    qt3dsdm::Qt3DSDMInstanceHandle GetInstanceFromSelectable(
            Q3DStudio::SSelectedValue inSelectedItem) const;
    qt3dsdm::Qt3DSDMInstanceHandle GetSelectedInstance() const;

    void CutSelectedObject();
    void DeleteSelectedItems(bool slide = false);
    void DeleteSelectedObject(bool slide = false);
    bool DeleteSelectedKeys();
    void SetChangedKeyframes();

    // Cut object to clipboard
    void CutObject(qt3dsdm::TInstanceHandleList inInstance);
    void CutObject(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    {
        qt3dsdm::TInstanceHandleList objects;
        objects.push_back(inInstance);
        CutObject(objects);
    }
    // copy object to clipboard
    void CopyObject(qt3dsdm::TInstanceHandleList inInstance);
    void CopyObject(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    {
        qt3dsdm::TInstanceHandleList objects;
        objects.push_back(inInstance);
        CopyObject(objects);
    }
    // paste object to clipboard
    qt3dsdm::Qt3DSDMInstanceHandle getPasteTarget(qt3dsdm::Qt3DSDMInstanceHandle selected) const;
    void PasteObject(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void PasteObjectMaster(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void DeleteObject(const qt3dsdm::TInstanceHandleList &inInstances);

    void DeleteObject(qt3dsdm::Qt3DSDMInstanceHandle inInstance)
    {
        qt3dsdm::TInstanceHandleList theInstances;
        theInstances.push_back(inInstance);
        DeleteObject(theInstances);
    }

    void ClientStep();

    void HandleCopy();
    void HandlePaste();
    void HandleMasterPaste();
    void HandleCut();

    bool CanCopyObject(); // specifically for objects
    bool CanCopyObject(const qt3dsdm::TInstanceHandleList &inInstances); // specifically for objects
    bool CanCopyKeyframe(); // specifically for keyframes
    bool CanCopyAction(); // specifically for actions
    bool CanPasteObject() const; // specifically for objects
    bool CanPasteKeyframe(); // specifically for keyframes
    bool CanPasteAction(); // specifically for actions

    bool CanPaste(); // for objects or keyframes or actions
    bool CanCopy(); // for objects or keyframes or actions
    bool CanCut(); // for objects or keyframes or actions
    void HandleDuplicateCommand(bool slide = false);

    bool VerifyCanRename(qt3dsdm::Qt3DSDMInstanceHandle inAsset);

    void DeselectAllItems(bool inSendEvent = true);

    qt3dsdm::Qt3DSDMInstanceHandle GetActiveLayer();
    void SetActiveLayer(qt3dsdm::Qt3DSDMInstanceHandle inLayerInstance);
    qt3dsdm::Qt3DSDMSlideHandle GetActiveSlide();
    void OnLayerDeleted(qt3dsdm::Qt3DSDMInstanceHandle inLayerInstance);

    void SetPlayMode(EPlayMode inPlayMode, long inRestoreTime = -1);
    bool IsPlaying();
    long GetCurrentClientTime();

    qt3dsdm::Qt3DSDMInstanceHandle GetSceneInstance() { return m_SceneInstance; }

    // IDoc
    virtual qt3dsdm::Qt3DSDMInstanceHandle GetActiveRootInstance();
    long GetCurrentViewTime() const override;
    virtual void OnComponentSeconds();
    // Notify time changed and set the playback clock to this time.
    void NotifyTimeChanged(long inNewTime) override;
    // Notify time changed.
    virtual void DoNotifyTimeChanged(long inNewTime);
    void NotifyActiveSlideChanged(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide) override;
    void NotifyActiveSlideChanged(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide,
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
    qt3dsdm::IPropertySystem *GetPropertySystem() override;
    qt3dsdm::IAnimationCore *GetAnimationCore() override;
    void SetInstancePropertyValue(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                  const QString &inPropertyName,
                                  const qt3dsdm::SValue &inValue) override;
    void SetInstancePropertyControlled(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                       const QString &instancepath,
                                       qt3dsdm::Qt3DSDMPropertyHandle propName,
                                       const QString &controller,
                                       bool controlled, bool batch = false) override;

    void RemoveDatainputBindings(
            const QMultiMap<QString, QPair<qt3dsdm::Qt3DSDMInstanceHandle,
                                           qt3dsdm::Qt3DSDMPropertyHandle>> *map) override;
    Q3DStudio::IDocumentBufferCache &GetBufferCache() override;
    Q3DStudio::IDocumentReader &GetDocumentReader() override;
    Q3DStudio::IDocumentEditor &OpenTransaction(const QString &inCmdName, const char *inFile,
                                                int inLine) override;
    Q3DStudio::IDocumentEditor &MaybeOpenTransaction(const QString &cmdName, const char *inFile,
                                                     int inLine) override;
    bool IsTransactionOpened() const override;
    void RollbackTransaction() override;
    void CloseTransaction() override;
    void IKnowWhatIAmDoingForceCloseTransaction() override;

    std::shared_ptr<Q3DStudio::IComposerSerializer> CreateSerializer() override;
    virtual std::shared_ptr<Q3DStudio::IComposerSerializer> CreateTransactionlessSerializer();
    // Create a DOM writer that is opened to the project element.  This is where the serializer
    // should write to.
    std::shared_ptr<qt3dsdm::IDOMWriter> CreateDOMWriter() override;
    // Create a DOM reader and check that the top element's version is correct.  Opens the reader
    // to the project element.
    virtual std::shared_ptr<qt3dsdm::IDOMReader>
    CreateDOMReader(const QString &inFilePath, qt3ds::QT3DSI32 &outVersion) override;
    virtual std::shared_ptr<qt3dsdm::IDOMReader> CreateDOMReader(QIODevice &inStream,
                                                                 qt3ds::QT3DSI32 &outVersion);

    void SelectDataModelObject(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle);
    // multiselect support
    void ToggleDataModelObjectToSelection(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle);
    void SelectAndNavigateToDataModelObject(qt3dsdm::Qt3DSDMInstanceHandle inInstanceHandle);
    long GetLatestEndTime();
    bool isPlayHeadAtEnd();

    CCore *GetCore() const override;

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
    Q3DStudio::CColor           GetEditViewBackgroundColor( );
    */

    qt3dsdm::CStudioSystem *GetStudioSystem() override { return m_StudioSystem.get(); }

    IObjectReferenceHelper *GetDataModelObjectReferenceHelper() const
    {
        return m_DataModelObjectRefHelper;
    }

    void SetDefaultKeyframeInterpolation(bool inSmooth);
    void ScheduleRemoveImageInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance, CCmdBatch *outBatch);
    void ScheduleRemoveDataModelInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                          CCmdBatch *outBatch);
    void ScheduleRemoveComponentInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                          CCmdBatch *outBatch);

    TAssetGraphPtr GetAssetGraph() { return m_AssetGraph; }
    virtual void AddToGraph(qt3dsdm::Qt3DSDMInstanceHandle inParentInstance,
                            qt3dsdm::Qt3DSDMInstanceHandle inInstance);

    // helper
    void IterateImageInstances(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                               std::vector<Q3DStudio::CId> *outImageIdList);
    qt3dsdm::Qt3DSDMInstanceHandle GetObjectbySelectMode(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                                         bool inGroupMode);

    // ICmdStackModifier
    bool CanUndo() override;
    bool PreUndo() override;

    void getSceneMaterials(qt3dsdm::Qt3DSDMInstanceHandle inParent,
                           QVector<qt3dsdm::Qt3DSDMInstanceHandle> &outMats) const;
    void CheckActionDependencies(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void SetActiveSlideWithTransaction(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide);

    void SetSceneGraph(std::shared_ptr<Q3DStudio::IDocSceneGraph> inGraph);
    Q3DStudio::IDocSceneGraph *GetSceneGraph() { return m_SceneGraph.get(); }

    void GetProjectFonts(std::vector<std::pair<QString, QString>> &outFontNameFileList);
    void GetProjectFonts(std::vector<QString> &outFonts);
     // Given a font file, return the font name
    QString GetProjectFontName(const QFileInfo &inFullPathToFontFile);
    void setPlayBackPreviewState(bool state);
    bool isPlayBackPreviewOn() const;
    int getSelectedInstancesCount() const;

    std::shared_ptr<Q3DStudio::IInternalDocumentEditor> getSceneEditor() { return m_SceneEditor; }
    QVector<qt3dsdm::Qt3DSDMInstanceHandle> getLayers();

protected:
    // Set the active slide, return true if delving
    void SetActiveSlideChange(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide);
    void OnSlideDeleted(qt3dsdm::Qt3DSDMSlideHandle inSlide);
    void OnInstanceDeleted(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    void onPropertyChanged(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                           qt3dsdm::Qt3DSDMPropertyHandle inProperty);
    Q3DStudio::SSelectedValue SetupInstanceSelection(qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    // Set the selection, but don't send an event.
    bool SetSelection(Q3DStudio::SSelectedValue inNewSelection = Q3DStudio::SSelectedValue());
    void LoadPresentationFile(QIODevice *inInputStream);
    void SavePresentationFile(QIODevice *inOutputStream);

    void CleanupData();
    void ResetData();
    int LoadStudioData(QIODevice *inInputStream);
    void ResetDataCore();
    void SetupDataCoreSignals();

    void CreatePresentation();
    void ClosePresentation();

    QString GetActionDependencies(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                  const QString &inActionDependencies);
    void GetActionDependencies(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                               qt3dsdm::TActionHandleList &ioActionList);

    qt3dsdm::Qt3DSDMInstanceHandle GetFirstSelectableLayer();
    qt3dsdm::Qt3DSDMInstanceHandle GetTopmostGroup(qt3dsdm::Qt3DSDMInstanceHandle inInstance);

    void GetActionsAffectedByRename(qt3dsdm::Qt3DSDMInstanceHandle inAsset,
                                    std::set<QString> &ioActionsAffected);

    bool isFocusOnTextEditControl();

    //==========================================================================
    //	Protected Fields
    //==========================================================================

    long m_PlayMode; ///< This tracks whether we're playing a client presentation or not.
    Q3DStudio::SSelectedValue m_SelectedObject;
    long m_CurrentViewTime; ///< The current time that is displayed by the playhead, not necessarily
                            ///the client time.
    qt3dsdm::Qt3DSDMInstanceHandle m_SceneInstance; ///< Pointer to the root level Scene object.
    qt3dsdm::Qt3DSDMSlideHandle m_ActiveSlide; ///< The currently active Slide Handle.
    qt3dsdm::Qt3DSDMInstanceHandle m_ActiveLayer; ///< The currently active layer.
    CPlaybackClock *m_PlaybackClock; ///< Playback clock. This is used when user clicks "Play"
    CCore *m_Core;
    bool m_IsModified;
    bool m_IsTemporary;
    QString m_DocumentPath;

    CDataManager *m_DataManager; ///< Manager for handling data properties.

    std::shared_ptr<qt3dsdm::CStudioSystem> m_StudioSystem;

    IKeyframesManager *m_KeyframesManager; ///< To support menu actions for selected keys

    IObjectReferenceHelper *m_DataModelObjectRefHelper; ///< To support object reference control

    TAssetGraphPtr m_AssetGraph;

    std::shared_ptr<Q3DStudio::IInternalDocumentEditor> m_SceneEditor;
    std::shared_ptr<SDocumentDataModelListener> m_DataModelListener;
    std::shared_ptr<Q3DStudio::IDocumentBufferCache> m_DocumentBufferCache;
    std::vector<std::shared_ptr<qt3dsdm::ISignalConnection>> m_Connections;
    std::shared_ptr<Q3DStudio::IDirectoryWatchingSystem> m_DirectoryWatchingSystem;
    std::shared_ptr<Q3DStudio::IImportFailedHandler> m_ImportFailedHandler;
    std::shared_ptr<Q3DStudio::IDeletingReferencedObjectHandler> m_DeletingReferencedObjectHandler;
    std::shared_ptr<Q3DStudio::IMoveRenameHandler> m_moveRenameHandler;
    long m_TransactionDepth;
    std::shared_ptr<qt3dsdm::CmdDataModel> m_OpenTransaction;
    std::shared_ptr<Q3DStudio::IDocSceneGraph> m_SceneGraph;
    Q3DStudio::SSelectedValue m_SelectedValue;
    bool m_nudging;
    bool m_unnotifiedSelectionChange = false;

public:
    void OnNewPresentation();
    void OnPresentationDeactivated();

protected:
    CRenderContext *m_RenderContext; ///< The render context attached to this player's window.
    Qt3DSRenderDevice m_WindowHandle; ///< The window handle to which to render
    Q3DStudio::CRect m_ClientSize;
    Q3DStudio::CRect m_SceneRect; ///< The dimensions of the active scene view

private:
    bool m_playbackPreviewOn = false;
    QString m_presentationId;
};

#endif // INCLUDED_DOC_H
