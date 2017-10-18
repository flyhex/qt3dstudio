/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "UICState.h"
#include "UICStateEditorEditorsImpl.h"
#include "UICStateExecutionContext.h"
#include "EASTL/sort.h"

using namespace uic::state;
using namespace uic::state::editor;

namespace uic {
namespace state {
    namespace editor {

#pragma warning(disable : 4355)

        SEditorImpl::SEditorImpl(TFoundationPtr inFoundation,
                                 NVScopedRefCounted<IStringTable> inStringTable)
            : m_EditorFoundation(inFoundation)
            , m_StringTable(inStringTable)
            , m_AutoAllocator(m_EditorFoundation->getFoundation())
            , m_StateContext(IStateContext::Create(m_EditorFoundation->getFoundation()))
            , m_Editors(m_EditorFoundation->getAllocator(), "SEditorImpl::m_Editors")
            , mRefCount(0)
            , m_TransactionManager(
                  QT3DS_NEW(inFoundation->getAllocator(), STransactionManagerImpl)(inFoundation))
            , m_Accessors(m_EditorFoundation->getAllocator(), "SEditorImpl::m_Accessors")
            , m_CopyPasteListener(NULL)
        {
        }

        void SEditorImpl::addRef() { atomicIncrement(&mRefCount); }

        void SEditorImpl::release()
        {
            QT3DSI32 count = atomicDecrement(&mRefCount);
            if (count <= 0) {
                TFoundationPtr theFoundation(m_EditorFoundation);
                NVDelete(theFoundation->getAllocator(), this);
            }
        }

        TObjPtr SEditorImpl::InsertEditor(void *inData, IEditorObject *inEditor)
        {
            if (inEditor) {
                bool insertResult = m_Editors.insert(eastl::make_pair(inData, inEditor)).second;
                QT3DS_ASSERT(insertResult);
                (void)insertResult;
            }
            return inEditor;
        }

        template <typename TStateType>
        TObjPtr SEditorImpl::ToEditor(TStateType &inItem)
        {
            TStateEditorMap::iterator iter = m_Editors.find(&inItem);
            if (iter != m_Editors.end())
                return iter->second;

            TObjPtr retval = SStateEditorMap<TStateType>::CreateEditor(inItem, *this, m_Accessors);
            if (retval)
                InsertEditor(&inItem, retval.mPtr);
            return retval;
        }

        template <typename TStateType>
        TObjPtr SEditorImpl::ToEditor(TStateType *inItem)
        {
            if (inItem == NULL)
                return TObjPtr();

            return ToEditor(*inItem);
        }

        TObjPtr SEditorImpl::ToEditor(SStateNode &inItem)
        {
            switch (inItem.m_Type) {
            case StateNodeTypes::State:
                return ToEditor(static_cast<SState &>(inItem));
            case StateNodeTypes::Parallel:
                return ToEditor(static_cast<SParallel &>(inItem));
            case StateNodeTypes::Transition:
                return ToEditor(static_cast<STransition &>(inItem));
            case StateNodeTypes::Final:
                return ToEditor(static_cast<SFinal &>(inItem));
            case StateNodeTypes::SCXML:
                return ToEditor(static_cast<SSCXML &>(inItem));
            case StateNodeTypes::History:
                return ToEditor(static_cast<SHistory &>(inItem));
            default:
                QT3DS_ASSERT(false);
                return TObjPtr();
            }
        }

        TObjPtr SEditorImpl::ExecutableContentToEditor(SExecutableContent &inItem)
        {
            TStateEditorMap::iterator iter = m_Editors.find(&inItem);
            if (iter != m_Editors.end())
                return iter->second;

            switch (inItem.m_Type) {
            case ExecutableContentTypes::Send:
                return ToEditor(static_cast<SSend &>(inItem));
            case ExecutableContentTypes::Raise:
                return ToEditor(static_cast<SRaise &>(inItem));
            case ExecutableContentTypes::Log:
                return ToEditor(static_cast<SLog &>(inItem));
            case ExecutableContentTypes::Assign:
                return ToEditor(static_cast<SAssign &>(inItem));
            case ExecutableContentTypes::If:
                return ToEditor(static_cast<SIf &>(inItem));
            case ExecutableContentTypes::ElseIf:
                return ToEditor(static_cast<SElseIf &>(inItem));
            case ExecutableContentTypes::Else:
                return ToEditor(static_cast<SElse &>(inItem));
            case ExecutableContentTypes::Script:
                return ToEditor(static_cast<SScript &>(inItem));
            case ExecutableContentTypes::Cancel:
                return ToEditor(static_cast<SCancel &>(inItem));
            default:
                QT3DS_ASSERT(false);
                return TObjPtr();
            }
        }

        template <typename TStateType>
        TStateType *SEditorImpl::FromEditor(TObjPtr inPtr)
        {
            if (inPtr.mPtr == NULL)
                return NULL;
            const char8_t *typeName = inPtr->TypeName();
            typedef typename SStateEditorMap<TStateType>::TEditorType TProspectiveType;
            if (AreEqual(typeName, TProspectiveType::GetTypeStr())) {
                return reinterpret_cast<TStateType *>(
                    &static_cast<TProspectiveType *>(inPtr.mPtr)->m_Data);
            }
            return NULL;
        }

        SStateNode *SEditorImpl::StateNodeFromEditor(TObjPtr inPtr)
        {
            if (inPtr.mPtr == NULL)
                return NULL;
            const char8_t *typeName = inPtr->TypeName();
            if (AreEqual(typeName, SSCXMLEditor::GetTypeStr()))
                return FromEditor<SSCXML>(inPtr);
            if (AreEqual(typeName, SStateEditor::GetTypeStr()))
                return FromEditor<SState>(inPtr);
            if (AreEqual(typeName, STransitionEditor::GetTypeStr()))
                return FromEditor<STransition>(inPtr);
            if (AreEqual(typeName, SParallelEditor::GetTypeStr()))
                return FromEditor<SParallel>(inPtr);
            if (AreEqual(typeName, SFinalEditor::GetTypeStr()))
                return FromEditor<SFinal>(inPtr);
            if (AreEqual(typeName, SHistoryEditor::GetTypeStr()))
                return FromEditor<SHistory>(inPtr);
            return NULL;
        }

        SExecutableContent *SEditorImpl::ExecutableContentFromEditor(TObjPtr inPtr)
        {
            if (inPtr.mPtr == NULL)
                return NULL;
            const char8_t *typeName = inPtr->TypeName();
            if (AreEqual(typeName, SSendEditor::GetTypeStr())) {
                return FromEditor<SSend>(inPtr);
            }
            if (AreEqual(typeName, SRaiseEditor::GetTypeStr())) {
                return FromEditor<SRaise>(inPtr);
            }
            if (AreEqual(typeName, SLogEditor::GetTypeStr())) {
                return FromEditor<SLog>(inPtr);
            }
            if (AreEqual(typeName, SAssignEditor::GetTypeStr())) {
                return FromEditor<SAssign>(inPtr);
            }
            if (AreEqual(typeName, SIfEditor::GetTypeStr())) {
                return FromEditor<SIf>(inPtr);
            }
            if (AreEqual(typeName, SElseIfEditor::GetTypeStr())) {
                return FromEditor<SElseIf>(inPtr);
            }
            if (AreEqual(typeName, SElseEditor::GetTypeStr())) {
                return FromEditor<SElse>(inPtr);
            }
            if (AreEqual(typeName, SScriptEditor::GetTypeStr())) {
                return FromEditor<SScript>(inPtr);
            }
            if (AreEqual(typeName, SCancelEditor::GetTypeStr())) {
                return FromEditor<SCancel>(inPtr);
            }

            return NULL;
        }

        void SEditorImpl::GenerateUniqueId(SStateNode &inNode, const char8_t *inStem)
        {
            QT3DS_ASSERT(inNode.m_Id.IsValid() == false);
            CXMLIO::GenerateUniqueId(inNode, inStem, *m_StateContext, *m_StringTable);
        }

        void SEditorImpl::GenerateUniqueId(SSend &inNode, const char8_t *inStem)
        {
            CXMLIO::GenerateUniqueId(inNode, inStem, *m_StateContext, *m_StringTable);
        }

        TObjPtr SEditorImpl::GetRoot() { return ToEditor(m_StateContext->GetRoot()); }

        template <typename TStateType>
        eastl::pair<TStateType *, TObjPtr> SEditorImpl::CreateEditorAndObject()
        {
            typedef typename SStateEditorMap<TStateType>::TEditorType TEditorType;
            TObjPtr newEditor = SStateEditorMap<TStateType>::CreateEditor(*this, m_Accessors);
            TStateType *theState = &static_cast<TEditorType *>(newEditor.mPtr)->m_Data;
            eastl::pair<TStateType *, TObjPtr> retval = eastl::make_pair(theState, newEditor);
            InsertEditor(retval.first, retval.second.mPtr);
            return retval;
        }

        TObjPtr SEditorImpl::DoCreate(const char8_t *inTypeName, const char8_t *inId)
        {
            if (AreEqual(inTypeName, SSCXMLEditor::GetTypeStr())) {
                QT3DS_ASSERT(m_StateContext->GetRoot() == NULL);
                eastl::pair<SSCXML *, TObjPtr> theNewEditor = CreateEditorAndObject<SSCXML>();
                GenerateUniqueId(*theNewEditor.first, inId == NULL ? "scxml" : inId);
                m_StateContext->SetRoot(*theNewEditor.first);
                return theNewEditor.second;
            } else if (AreEqual(inTypeName, SStateEditor::GetTypeStr())) {
                eastl::pair<SState *, TObjPtr> theNewEditor = CreateEditorAndObject<SState>();
                GenerateUniqueId(*theNewEditor.first, inId == NULL ? "state" : inId);
                return theNewEditor.second;
            } else if (AreEqual(inTypeName, SParallelEditor::GetTypeStr())) {
                eastl::pair<SParallel *, TObjPtr> theNewEditor = CreateEditorAndObject<SParallel>();
                GenerateUniqueId(*theNewEditor.first, inId == NULL ? "parallel" : inId);
                return theNewEditor.second;
            } else if (AreEqual(inTypeName, SFinalEditor::GetTypeStr())) {
                eastl::pair<SFinal *, TObjPtr> theNewEditor = CreateEditorAndObject<SFinal>();
                GenerateUniqueId(*theNewEditor.first, inId == NULL ? "final" : inId);
                return theNewEditor.second;
            } else if (AreEqual(inTypeName, SHistoryEditor::GetTypeStr())) {
                eastl::pair<SHistory *, TObjPtr> theNewEditor = CreateEditorAndObject<SHistory>();
                GenerateUniqueId(*theNewEditor.first, inId == NULL ? "history" : inId);
                return theNewEditor.second;
            } else if (AreEqual(inTypeName, STransitionEditor::GetTypeStr())) {
                eastl::pair<STransition *, TObjPtr> theNewEditor =
                    CreateEditorAndObject<STransition>();
                GenerateUniqueId(*theNewEditor.first, inId == NULL ? "transition" : inId);
                return theNewEditor.second;
            } else if (AreEqual(inTypeName, SDataModelEditor::GetTypeStr())) {
                eastl::pair<SDataModel *, TObjPtr> theNewEditor =
                    CreateEditorAndObject<SDataModel>();
                return theNewEditor.second;
            } else if (AreEqual(inTypeName, SDataEditor::GetTypeStr())) {
                eastl::pair<SData *, TObjPtr> theNewEditor = CreateEditorAndObject<SData>();
                return theNewEditor.second;
            }

            QT3DS_ASSERT(false);
            return TObjPtr();
        }

        TObjPtr SEditorImpl::Create(const char8_t *inTypeName, const char8_t *inId)
        {
            TObjPtr retval = DoCreate(inTypeName, inId);
            m_TransactionManager->OnObjectCreated(retval);
            return retval;
        }

        TObjPtr SEditorImpl::GetOrCreate(SSCXML &inData) { return ToEditor(inData); }
        TObjPtr SEditorImpl::GetOrCreate(SState &inData) { return ToEditor(inData); }
        TObjPtr SEditorImpl::GetOrCreate(STransition &inData) { return ToEditor(inData); }
        TObjPtr SEditorImpl::GetOrCreate(SParallel &inData) { return ToEditor(inData); }
        TObjPtr SEditorImpl::GetOrCreate(SFinal &inData) { return ToEditor(inData); }
        TObjPtr SEditorImpl::GetOrCreate(SHistory &inData) { return ToEditor(inData); }
        TObjPtr SEditorImpl::GetOrCreate(SDataModel &inData) { return ToEditor(inData); }
        TObjPtr SEditorImpl::GetOrCreate(SData &inData) { return ToEditor(inData); }

        TObjPtr SEditorImpl::GetObjectById(const char8_t *inId)
        {
            if (isTrivial(inId))
                return TObjPtr();
            SStateNode *theNode = m_StateContext->FindStateNode(RegisterStr(inId));
            if (theNode)
                return ToEditor(*theNode);
            return TObjPtr();
        }

        TObjPtr SEditorImpl::GetEditor(void *inGraphData)
        {
            TStateEditorMap::iterator iter = m_Editors.find(inGraphData);
            if (iter != m_Editors.end())
                return iter->second;
            return TObjPtr();
        }

        TSignalConnectionPtr SEditorImpl::AddChangeListener(IEditorChangeListener &inListener)
        {
            return m_TransactionManager->AddChangeListener(inListener);
        }

        TTransactionPtr SEditorImpl::BeginTransaction(const TEditorStr &inName)
        {
            return m_TransactionManager->BeginTransaction(inName);
        }

        TTransactionPtr SEditorImpl::GetOpenTransaction()
        {
            return m_TransactionManager->GetOpenTransaction();
        }

        void SEditorImpl::RollbackTransaction() { m_TransactionManager->RollbackTransaction(); }

        void SEditorImpl::EndTransaction() { m_TransactionManager->EndTransaction(); }

        TEditorStr SEditorImpl::Copy(TObjList inObjects, const QT3DSVec2 &inMousePos)
        {
            eastl::vector<SStateNode *> theNodeList;
            for (size_t idx = 0, end = inObjects.size(); idx < end; ++idx) {
                SStateNode *theNode = StateNodeFromEditor(inObjects[idx]);
                if (theNode && theNode->m_Type != StateNodeTypes::Transition
                    && theNode->m_Type != StateNodeTypes::SCXML) {
                    theNodeList.push_back(theNode);
                }
            }
            eastl::vector<SNamespacePair> theNamespaces;
            NVScopedRefCounted<IDOMFactory> theFactory(
                IDOMFactory::CreateDOMFactory(m_EditorFoundation->getAllocator(), m_StringTable));
            NVScopedRefCounted<IDOMWriter> theWriter(
                IDOMWriter::CreateDOMWriter(m_EditorFoundation->getAllocator(), "scxml_fragment",
                                            m_StringTable)
                    .first);

            eastl::vector<SStateNode *> rootNodes = CXMLIO::SaveSCXMLFragment(
                *m_StateContext, *m_EditorFoundation->m_Foundation, *m_StringTable, *theWriter,
                toDataRef(theNodeList.data(), theNodeList.size()), *this, inMousePos,
                theNamespaces);

            if (m_CopyPasteListener)
                m_CopyPasteListener->OnCopy(this, rootNodes, *theWriter, theNamespaces);

            SEditorImplStrIOStream theStream;
            CDOMSerializer::WriteXMLHeader(theStream);
            CDOMSerializer::Write(m_EditorFoundation->getAllocator(), *theWriter->GetTopElement(),
                                  theStream, *m_StringTable,
                                  toDataRef(theNamespaces.data(), theNamespaces.size()), false);
            return theStream.m_Str;
        }

        bool SEditorImpl::CanPaste(TObjPtr inTarget)
        {
            SStateNode *theNode = StateNodeFromEditor(inTarget);
            if (theNode) {
                return theNode->m_Type != StateNodeTypes::Transition
                    && theNode->m_Type != StateNodeTypes::History
                    && theNode->m_Type != StateNodeTypes::Final;
            }

            return false;
        }

        void SEditorImpl::AddNewPasteObjectToTransaction(SStateNode &inNode)
        {
            TObjPtr editorPtr = ToEditor(inNode);
            if (editorPtr) {
                m_TransactionManager->m_Transaction->m_Changes.push_back(new SChange(*editorPtr));
                m_TransactionManager->OnObjectCreated(editorPtr);
                Option<SValue> children = editorPtr->GetPropertyValue("children");
                if (children.hasValue()) {
                    TObjList childList = children->getData<TObjList>();
                    for (size_t idx = 0, end = childList.size(); idx < end; ++idx) {
                        SStateNode *childNode = StateNodeFromEditor(childList[idx]);
                        if (childNode)
                            AddNewPasteObjectToTransaction(*childNode);
                    }
                }
            }
        }

        void RecurseAndCheckForValidHistoryAfterPaste(TObjPtr inNode, SEditorImpl &inEditor)
        {
            if (AreEqual(inNode->TypeName(), "history")) {
                inEditor.CheckAndSetValidHistoryDefault(inNode);
            } else {
                Option<SValue> childrenOpt = inNode->GetPropertyValue("children");
                if (childrenOpt.hasValue() == false) {
                    return;
                }
                TObjList children = childrenOpt->getData<TObjList>();
                for (size_t childIdx = 0, childEnd = children.size(); childIdx < childEnd;
                     ++childIdx) {
                    RecurseAndCheckForValidHistoryAfterPaste(children[childIdx], inEditor);
                }
            }
        }

        void SEditorImpl::Paste(const TEditorStr &inCopiedObjects, TObjPtr inTarget,
                                const QT3DSVec2 &inMousePos)
        {
            Option<SValue> childrenOpt = inTarget->GetPropertyValue("children");
            if (childrenOpt.hasValue() == false) {
                QT3DS_ASSERT(false);
                return;
            }

            TObjList children = childrenOpt->getData<TObjList>();

            SEditorImplStrInStream inStream(inCopiedObjects);
            NVScopedRefCounted<IDOMFactory> theFactory =
                IDOMFactory::CreateDOMFactory(m_EditorFoundation->getAllocator(), m_StringTable);
            eastl::pair<SNamespacePairNode *, SDOMElement *> readResult =
                CDOMSerializer::Read(*theFactory, inStream);
            SDOMElement *elem = readResult.second;
            if (elem == NULL) {
                QT3DS_ASSERT(false);
                return;
            }
            NVScopedRefCounted<IDOMReader> theReader = IDOMReader::CreateDOMReader(
                m_EditorFoundation->getAllocator(), *elem, m_StringTable, *theFactory);
            eastl::pair<eastl::vector<SStateNode *>, CXMLIO::TIdRemapMap> theRootsPair;
            {
                IDOMReader::Scope __readerScope(*theReader);
                theRootsPair =
                    CXMLIO::LoadSCXMLFragment(m_AutoAllocator, *m_EditorFoundation->m_Foundation,
                                              *theReader, *m_StringTable, *m_StateContext, *this);
            }
            eastl::vector<SStateNode *> &theObjects(theRootsPair.first);
            if (m_CopyPasteListener)
                m_CopyPasteListener->OnPaste(this, *theReader, theRootsPair.second);
            // Merge any namespaces into the context's namespace list.
            if (m_StateContext->GetDOMFactory()) {
                for (SNamespacePairNode *fragNode = readResult.first; fragNode;
                     fragNode = fragNode->m_NextNode) {
                    bool found = false;
                    for (SNamespacePairNode *ctxNode = m_StateContext->GetFirstNSNode();
                         ctxNode && !found; ctxNode = ctxNode->m_NextNode) {
                        if (ctxNode->m_Namespace == fragNode->m_Namespace)
                            found = true;
                    }
                    if (!found) {
                        SNamespacePairNode *newNode =
                            m_StateContext->GetDOMFactory()->NextNSPairNode(
                                fragNode->m_Namespace, fragNode->m_Abbreviation);
                        newNode->m_NextNode = m_StateContext->GetFirstNSNode();
                        m_StateContext->SetFirstNSNode(*newNode);
                    }
                }
            }
            QT3DSVec2 globalPos(0, 0);
            if (inTarget) {
                for (TObjPtr posObj = inTarget; posObj; posObj = posObj->Parent()) {
                    Option<SValue> theData = posObj->GetPropertyValue("position");
                    if (theData.hasValue())
                        globalPos += theData->getData<QT3DSVec2>();
                }
            }
            QT3DSVec2 theMousePos = inMousePos - globalPos;
            for (size_t idx = 0, end = theObjects.size(); idx < end; ++idx) {
                SStateNode *currentObject = theObjects[idx];
                QT3DSVec2 objPos(0, 0);
                if (currentObject->m_StateNodeFlags.HasPosition())
                    objPos = currentObject->m_Position;
                currentObject->SetPosition(objPos + theMousePos);
                if (m_TransactionManager->m_Transaction)
                    AddNewPasteObjectToTransaction(*currentObject);
                TObjPtr editorPtr = ToEditor(*currentObject);
                if (editorPtr.mPtr)
                    children.push_back(editorPtr);
            }
            // This sets up the valid parents.  Without that, checking if a history node is valid
            // is meaningless.
            inTarget->SetPropertyValue("children", children);
            for (size_t idx = 0, end = children.size(); idx < end; ++idx) {
                RecurseAndCheckForValidHistoryAfterPaste(children[idx], *this);
            }
        }

        bool SEditorImpl::IsDerivedFrom(SStateNode &child, SStateNode &parent)
        {
            if (&child == &parent)
                return true;
            if (child.m_Parent)
                return IsDerivedFrom(*child.m_Parent, parent);
            return false;
        }

        // This method should always return a value because the scxml root item
        // is the parent of everyone else.
        SStateNode &SEditorImpl::GetLeastCommonAncestor(SStateNode &lhs, SStateNode &rhs)
        {
            // If lhs is derived from rhs, return rhs
            if (IsDerivedFrom(lhs, rhs))
                return rhs;
            // vice versa
            else if (IsDerivedFrom(rhs, lhs))
                return lhs;

            // Else wander up the tree till we find a common ancestor.
            QT3DS_ASSERT(lhs.m_Parent != NULL);
            if (lhs.m_Parent != NULL)
                return GetLeastCommonAncestor(*lhs.m_Parent, rhs);

            return lhs;
        }

        TObjPtr SEditorImpl::GetLeastCommonAncestor(NVConstDataRef<TObjPtr> inObjects)
        {
            SStateNode *lastAncestor = NULL;
            if (inObjects.size() == 0)
                return TObjPtr();

            for (QT3DSU32 idx = 0, end = inObjects.size(); idx < end; ++idx) {
                if (lastAncestor == NULL) {
                    lastAncestor = StateNodeFromEditor(inObjects[idx]);
                } else {
                    SStateNode *nextNode = StateNodeFromEditor(inObjects[idx]);
                    if (nextNode != NULL) {
                        lastAncestor = &GetLeastCommonAncestor(*lastAncestor, *nextNode);
                    }
                }
            }

            if (lastAncestor)
                return ToEditor(*lastAncestor);
            else
                return TObjPtr();
        }

        void GetCancelableSendIds(TExecutableContentList &inList, TObjList &outList,
                                  SEditorImpl &inEditor);

        void RecursiveGetCancelableSendIds(SExecutableContent &inNode, TObjList &outList,
                                           SEditorImpl &inEditor)
        {
            if (inNode.m_Type == ExecutableContentTypes::Send) {
                SSend &theSend = static_cast<SSend &>(inNode);
                if (IExecutionContext::ParseTimeStrToMilliseconds(theSend.m_Delay)
                    || !isTrivial(theSend.m_DelayExpr)) {
                    outList.push_back(inEditor.ExecutableContentToEditor(theSend));
                }
            }
            GetCancelableSendIds(inNode.m_Children, outList, inEditor);
        }

        void GetCancelableSendIds(TExecutableContentList &inList, TObjList &outList,
                                  SEditorImpl &inEditor)
        {
            for (TExecutableContentList::iterator iter = inList.begin(), end = inList.end();
                 iter != end; ++iter)
                RecursiveGetCancelableSendIds(*iter, outList, inEditor);
        }

        void RecursiveGetCancelableSendIds(SStateNode &inNode, TObjList &outList,
                                           SEditorImpl &inEditor)
        {
            if (inNode.m_Type == StateNodeTypes::Transition) {
                STransition &transition = static_cast<STransition &>(inNode);
                GetCancelableSendIds(transition.m_ExecutableContent, outList, inEditor);
            } else {
                TOnEntryList *entryList = inNode.GetOnEntryList();
                TOnExitList *exitList = inNode.GetOnExitList();
                if (entryList) {
                    for (TOnEntryList::iterator iter = entryList->begin(), end = entryList->end();
                         iter != end; ++iter)
                        GetCancelableSendIds(iter->m_ExecutableContent, outList, inEditor);
                }
                if (exitList) {
                    for (TOnExitList::iterator iter = exitList->begin(), end = exitList->end();
                         iter != end; ++iter)
                        GetCancelableSendIds(iter->m_ExecutableContent, outList, inEditor);
                }
            }
            TStateNodeList *childList = inNode.GetChildren();
            if (childList) {
                for (TStateNodeList::iterator childIter = childList->begin(),
                                              childEnd = childList->end();
                     childIter != childEnd; ++childIter)
                    RecursiveGetCancelableSendIds(*childIter, outList, inEditor);
            }
        }

        // TODO - think about a fast and easy way to cache things (and keep the cache correct).
        TObjList SEditorImpl::GetCancelableSendIds()
        {
            TObjList retval;
            if (m_StateContext->GetRoot())
                RecursiveGetCancelableSendIds(*m_StateContext->GetRoot(), retval, *this);
            return retval;
        }

        static const char8_t *whitespace = "\n\r\t ";

        void StripIfLastChar(TEditorStr &str, char data)
        {
            if (str.size() && str.back() == data)
                str.resize(str.size() - 1);
        }

        void AddUnique(TEditorStrList &list, const TEditorStr &str)
        {
            if (eastl::find(list.begin(), list.end(), str) == list.end())
                list.push_back(str);
        }

        void RecursiveScanExecutableContentForEvents(TExecutableContentList &contentList,
                                                     TEditorStrList &ioStrList)
        {
            for (TExecutableContentList::iterator iter = contentList.begin(),
                                                  end = contentList.end();
                 iter != end; ++iter) {
                if (iter->m_Type == ExecutableContentTypes::Send) {
                    SSend &theSend = static_cast<SSend &>(*iter);
                    if (!isTrivial(theSend.m_Event)) {
                        AddUnique(ioStrList, theSend.m_Event.c_str());
                    }
                } else if (iter->m_Type == ExecutableContentTypes::Raise) {
                    SRaise &theRaise = static_cast<SRaise &>(*iter);
                    if (!isTrivial(theRaise.m_Event)) {
                        AddUnique(ioStrList, theRaise.m_Event.c_str());
                    }
                }
                RecursiveScanExecutableContentForEvents(iter->m_Children, ioStrList);
            }
        }

        template <typename TListType>
        void ScanItemListForEvents(TListType *itemList, TEditorStrList &ioEvents)
        {
            if (itemList) {
                typedef typename TListType::iterator iterator;
                for (iterator iter = itemList->begin(), end = itemList->end(); iter != end;
                     ++iter) {
                    RecursiveScanExecutableContentForEvents(iter->m_ExecutableContent, ioEvents);
                }
            }
        }

        void RecursiveGetStateMachineEvents(SStateNode &inNode, TEditorStrList &ioList)
        {
            if (inNode.m_Type == StateNodeTypes::Transition) {
                STransition &theTransition = static_cast<STransition &>(inNode);
                TEditorStr transStr(theTransition.m_Event);
                for (size_t pos = transStr.find_first_not_of(whitespace); pos != TEditorStr::npos;
                     pos = transStr.find_first_not_of(whitespace, pos)) {
                    size_t end = transStr.find_first_of(whitespace, pos);
                    if (end == TEditorStr::npos)
                        end = transStr.size();
                    TEditorStr subStr = transStr.substr(pos, end - pos);
                    pos = end;
                    StripIfLastChar(subStr, '*');
                    StripIfLastChar(subStr, '.');
                    AddUnique(ioList, subStr);
                }
                RecursiveScanExecutableContentForEvents(theTransition.m_ExecutableContent, ioList);
            } else {
                ScanItemListForEvents(inNode.GetOnEntryList(), ioList);
                ScanItemListForEvents(inNode.GetOnExitList(), ioList);
            }
            TStateNodeList *children = inNode.GetChildren();
            if (children) {
                for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                     iter != end; ++iter)
                    RecursiveGetStateMachineEvents(*iter, ioList);
            }
        }

        TEditorStrList SEditorImpl::GetStateMachineEvents()
        {
            TEditorStrList retval;
            if (m_StateContext->GetRoot())
                RecursiveGetStateMachineEvents(*m_StateContext->GetRoot(), retval);
            eastl::sort(retval.begin(), retval.end());
            return retval;
        }

        eastl::pair<SExecutableContent *, TObjPtr>
        SEditorImpl::CreateExecutableContent(const char8_t *inTypeName)
        {
            SExecutableContent *newExecutable = NULL;
            TObjPtr newObj;
            if (AreEqual(inTypeName, SSendEditor::GetTypeStr())) {
                eastl::pair<SSend *, TObjPtr> theNewEditor = CreateEditorAndObject<SSend>();
                GenerateUniqueId(*theNewEditor.first, "send");
                newExecutable = theNewEditor.first;
                newObj = theNewEditor.second;
            } else if (AreEqual(inTypeName, SScriptEditor::GetTypeStr())) {
                eastl::pair<SScript *, TObjPtr> theNewEditor = CreateEditorAndObject<SScript>();
                newExecutable = theNewEditor.first;
                newObj = theNewEditor.second;
            } else if (AreEqual(inTypeName, SCancelEditor::GetTypeStr())) {
                eastl::pair<SCancel *, TObjPtr> theNewEditor = CreateEditorAndObject<SCancel>();
                newExecutable = theNewEditor.first;
                newObj = theNewEditor.second;
            } else {
                QT3DS_ASSERT(false);
            }
            return eastl::make_pair(newExecutable, newObj);
        }

        struct SExecListAddChange : public IChange
        {
            TExecutableContentList &parentList;
            SExecutableContent *prevSibling;
            SExecutableContent &item;
            TObjPtr editorObj;
            QT3DSI32 mRefCount;
            bool m_AddOnDo;
            SExecListAddChange(TExecutableContentList &plist, SExecutableContent *prev,
                               SExecutableContent &_item, TObjPtr objPtr, bool addOnDo)
                : parentList(plist)
                , prevSibling(prev)
                , item(_item)
                , editorObj(objPtr)
                , mRefCount(0)
                , m_AddOnDo(addOnDo)
            {
            }

            void Add()
            {
                if (prevSibling)
                    parentList.insert_after(*prevSibling, item);
                else
                    parentList.push_front(item);
            }
            void Remove() { parentList.remove(item); }

            virtual void Do()
            {
                if (m_AddOnDo)
                    Add();
                else
                    Remove();
            }

            virtual void Undo()
            {
                if (m_AddOnDo)
                    Remove();
                else
                    Add();
            }

            virtual TObjPtr GetEditor() { return editorObj; }

            virtual void addRef() { ++mRefCount; }

            virtual void release()
            {
                --mRefCount;
                if (mRefCount <= 0)
                    delete this;
            }
        };

        void SEditorImpl::ReplaceExecutableContent(SExecutableContent &oldContent,
                                                   SExecutableContent &newContent)
        {
            SStateNode *stateNodeParent = oldContent.m_StateNodeParent;
            SExecutableContent *execContentParent = oldContent.m_Parent;
            SExecutableContentParentInfo parentInfo(oldContent, *this);
            SExecutableContent *prevSibling = oldContent.m_PreviousSibling;

            // Can't use remove object from graph here because it is too aggressive.
            NVScopedRefCounted<SExecListAddChange> oldChange = new SExecListAddChange(
                *parentInfo.parentList, prevSibling, oldContent, parentInfo.editorObj, false);
            oldChange->Do();
            newContent.m_StateNodeParent = stateNodeParent;
            newContent.m_Parent = execContentParent;
            NVScopedRefCounted<SExecListAddChange> newChange = new SExecListAddChange(
                *parentInfo.parentList, prevSibling, newContent, parentInfo.editorObj, true);
            newChange->Do();
            if (GetOpenTransactionImpl()) {
                GetOpenTransactionImpl()->m_Changes.push_back(oldChange.mPtr);
                GetOpenTransactionImpl()->m_Changes.push_back(newChange.mPtr);
            }
        }

        TObjPtr SEditorImpl::ChangeExecutableContentType(TObjPtr inContent, const char8_t *newType)
        {
            SExecutableContent *theContent = ExecutableContentFromEditor(inContent);
            if (!theContent) {
                QT3DS_ASSERT(false);
                return TObjPtr();
            }
            SExecutableContentParentInfo parentInfo(*theContent, *this);
            // find the index of the item in the parent list.
            if (parentInfo.parentList == NULL) {
                QT3DS_ASSERT(false);
                return TObjPtr();
            }

            eastl::pair<SExecutableContent *, TObjPtr> theNewContent =
                CreateExecutableContent(newType);
            if (!theNewContent.first) {
                QT3DS_ASSERT(false);
                return TObjPtr();
            }
            ReplaceExecutableContent(*theContent, *theNewContent.first);
            return theNewContent.second;
        }

        TEditorStr SEditorImpl::ToXML(TObjPtr inContent)
        {
            SExecutableContent *theContent = ExecutableContentFromEditor(inContent);
            if (!theContent) {
                QT3DS_ASSERT(false);
                return TEditorStr();
            }
            NVScopedRefCounted<IDOMFactory> theFactory(
                IDOMFactory::CreateDOMFactory(m_EditorFoundation->getAllocator(), m_StringTable));
            NVScopedRefCounted<IDOMWriter> theWriter(
                IDOMWriter::CreateDOMWriter(m_EditorFoundation->getAllocator(), "innerXML",
                                            m_StringTable, 0)
                    .first);
            CXMLIO::ToEditableXml(*m_StateContext, m_EditorFoundation->getFoundation(),
                                  *m_StringTable, *theWriter, *theContent, *this);
            SDOMElement *theTopElem = theWriter->GetTopElement();
            SDOMElement *xmlElem = &theTopElem->m_Children.front();
            SEditorImplStrIOStream theStream;
            if (xmlElem) {

                eastl::vector<SNamespacePair> theNamespaces;
                for (SNamespacePairNode *theNode = m_StateContext->GetFirstNSNode(); theNode;
                     theNode = theNode->m_NextNode)
                    theNamespaces.push_back(*theNode);
                if (theNamespaces.empty()) {
                    theNamespaces.push_back(SNamespacePair(
                        m_StringTable->RegisterStr("http://www.w3.org/2005/07/scxml")));
                    theNamespaces.push_back(
                        SNamespacePair(m_StringTable->RegisterStr("http://qt.io/qt3dstudio/uicstate"),
                                       m_StringTable->RegisterStr("uic")));
                }
                CDOMSerializer::Write(
                    m_EditorFoundation->getAllocator(), *xmlElem, theStream, *m_StringTable,
                    toDataRef(theNamespaces.data(), theNamespaces.size()), false, 0, false);
            }
            return theStream.m_Str;
        }

        struct SStrXMLErrorHandler : public CXmlErrorHandler
        {
            TEditorStr m_Error;
            virtual void OnXmlError(TXMLCharPtr errorName, int line, int /*column*/)
            {
                char buf[256];
                sprintf(buf, "Error on line %d: ", line);
                m_Error.assign(buf);
                m_Error.append(nonNull(errorName));
            }
        };

        eastl::pair<TEditorStr, TObjPtr> SEditorImpl::FromXML(TObjPtr inContent,
                                                              const TEditorStr &ioEditedXML)
        {
            TEditorStr wrappedXML("<snippet ");
            eastl::vector<SNamespacePair> theNamespaces;
            for (SNamespacePairNode *theNode = m_StateContext->GetFirstNSNode(); theNode;
                 theNode = theNode->m_NextNode)
                theNamespaces.push_back(*theNode);
            if (theNamespaces.empty()) {
                theNamespaces.push_back(
                    SNamespacePair(m_StringTable->RegisterStr("http://www.w3.org/2005/07/scxml")));
                theNamespaces.push_back(
                    SNamespacePair(m_StringTable->RegisterStr("http://qt.io/qt3dstudio/uicstate"),
                                   m_StringTable->RegisterStr("uic")));
            }
            for (size_t idx = 0, end = theNamespaces.size(); idx < end; ++idx) {
                SNamespacePair &theNode(theNamespaces[idx]);
                wrappedXML.append(" xmlns");
                if (theNode.m_Abbreviation.IsValid()) {
                    wrappedXML.append(":");
                    wrappedXML.append(theNode.m_Abbreviation.c_str());
                }
                wrappedXML.append("=\"");
                wrappedXML.append(theNode.m_Namespace.c_str());
                wrappedXML.append("\"");
            }
            wrappedXML.append(">");
            wrappedXML.append(ioEditedXML);
            wrappedXML.append("</snippet>");
            SEditorImplStrInStream theStream(wrappedXML);

            NVScopedRefCounted<IDOMFactory> theFactory(
                IDOMFactory::CreateDOMFactory(m_EditorFoundation->getAllocator(), m_StringTable));
            SStrXMLErrorHandler errorHandler;
            SDOMElement *theElem =
                CDOMSerializer::Read(*theFactory, theStream, &errorHandler).second;
            TObjPtr retval;
            if (theElem) {
                NVScopedRefCounted<IDOMReader> theReader(IDOMReader::CreateDOMReader(
                    m_EditorFoundation->getAllocator(), theElem->m_Children.front(), m_StringTable,
                    theFactory));
                SExecutableContent *oldContent = ExecutableContentFromEditor(inContent);
                SExecutableContent *theContent =
                    CXMLIO::FromEditableXML(*theReader, m_EditorFoundation->getFoundation(),
                                            *m_StateContext, *m_StringTable, m_AutoAllocator, *this,
                                            oldContent->m_StateNodeParent, oldContent->m_Parent);
                if (theContent) {
                    ReplaceExecutableContent(*oldContent, *theContent);
                    retval = ExecutableContentToEditor(*theContent);
                } else {
                    errorHandler.m_Error.assign("The \"");
                    errorHandler.m_Error.append(theReader->GetElementName().c_str());
                    errorHandler.m_Error.append("\" action is not recognized. It must be one of:");
                    eastl::vector<eastl::string> contentNames =
                        CXMLIO::GetSupportedExecutableContentNames();
                    for (size_t idx = 0, end = contentNames.size(); idx < end; ++idx) {
                        if (idx != 0)
                            errorHandler.m_Error.append(",");

                        errorHandler.m_Error.append(" \"");
                        errorHandler.m_Error.append(contentNames[idx].c_str());
                        errorHandler.m_Error.append("\"");
                    }
                    errorHandler.m_Error.append(".");
                }
            }

            return eastl::make_pair(errorHandler.m_Error, retval);
        }

        void SEditorImpl::ReleaseEditor(void *inData)
        {
            TStateEditorMap::iterator iter = m_Editors.find(inData);
            if (iter != m_Editors.end())
                m_Editors.erase(iter);
        }

        bool SEditorImpl::Save(const char8_t *inFileName)
        {
            CFileSeekableIOStream theFile(inFileName, FileWriteFlags());
            if (theFile.IsOpen() == false) {
                QT3DS_ASSERT(false);
                return false;
            }

            Save(theFile);
            return true;
        }

        void SEditorImpl::Save(IOutStream &inStream) { m_StateContext->Save(inStream, this); }

        bool SEditorImpl::Load(IInStream &inStream, const char8_t *inFilename)
        {
            bool theRetval = false;
            m_StateContext = IStateContext::Load(m_AutoAllocator, *m_EditorFoundation->m_Foundation,
                                                 inStream, inFilename, m_StringTable.mPtr, this);
            if (m_StateContext.mPtr == 0)
                m_StateContext = IStateContext::Create(*m_EditorFoundation->m_Foundation);
            else
                theRetval = true;
            return theRetval;
        }

        STransaction *SEditorImpl::GetOpenTransactionImpl()
        {
            return m_TransactionManager->m_Transaction.mPtr;
        }

        void SEditorImpl::SetIdProperty(SStateNode &inNode, const SValue &inData,
                                        CRegisteredString & /*inTarget*/)
        {
            IStateContext &theContext = *m_StateContext;
            theContext.EraseId(inNode.m_Id);
            TEditorStr potentialId = inData.getData<TEditorStr>();
            if (potentialId.size() == 0) {
                switch (inNode.m_Type) {
                case StateNodeTypes::State:
                    potentialId = "state";
                    break;
                case StateNodeTypes::Parallel:
                    potentialId = "parallel";
                    break;
                case StateNodeTypes::Transition:
                    potentialId = "transition";
                    break;
                case StateNodeTypes::Final:
                    potentialId = "final";
                    break;
                case StateNodeTypes::History:
                    potentialId = "history";
                    break;
                case StateNodeTypes::SCXML:
                    potentialId = "scxml";
                    break;
                default:
                    QT3DS_ASSERT(false);
                    potentialId = "id";
                    break;
                }
            }
            CRegisteredString oldId = inNode.m_Id;
            inNode.m_Id = CRegisteredString();
            GenerateUniqueId(inNode, potentialId.c_str());
            // This is a bad hack to get around:
            // 1. user changes id calling this function
            if (m_CopyPasteListener) {
                m_CopyPasteListener->OnIDChange(this, inNode, oldId);
            }
        }

        void SEditorImpl::SetIdProperty(SSend &inNode, const SValue &inData,
                                        CRegisteredString & /*inTarget*/)
        {
            IStateContext &theContext = *m_StateContext;
            theContext.EraseId(inNode.m_Id);
            TEditorStr potentialId = inData.getData<TEditorStr>();
            if (potentialId.size() == 0)
                potentialId = "send";

            inNode.m_Id = CRegisteredString();
            GenerateUniqueId(inNode, potentialId.c_str());
        }

        void SEditorImpl::Set(const Option<SValue> &inData, NVConstDataRef<SStateNode *> &inTarget)
        {
            if (inTarget.mData != NULL) {
                m_AutoAllocator.deallocate((void *)inTarget.mData);
                inTarget.mData = NULL;
                inTarget.mSize = 0;
            }
            if (inData.hasValue()) {
                TObjList inList(inData->getData<TObjList>());
                eastl::vector<SStateNode *> validObjects;
                for (size_t idx = 0, end = inList.size(); idx < end; ++idx) {
                    TObjPtr theObj = inList[idx];
                    SStateNode *theNode = StateNodeFromEditor(theObj);
                    if (theNode) {
                        validObjects.push_back(theNode);
                    } else {
                        QT3DS_ASSERT(false);
                    }
                }

                if (validObjects.empty() == false) {
                    NVConstDataRef<SStateNode *> &outList(inTarget);
                    size_t byteCount = validObjects.size() * sizeof(SStateNode *);
                    outList.mData = (SStateNode * const *)m_AutoAllocator.allocate(
                        byteCount, "graph ref list data", __FILE__, __LINE__);
                    memCopy((void *)outList.mData, validObjects.data(), byteCount);
                    outList.mSize = validObjects.size();
                }
            }
        }

        SValue SEditorImpl::Get(NVConstDataRef<SStateNode *> &inTarget)
        {
            TObjList retval;
            NVConstDataRef<SStateNode *> inRefList(inTarget);
            for (QT3DSU32 idx = 0, end = inRefList.size(); idx < end; ++idx) {
                TObjPtr obj = ToEditor(inRefList[idx]);
                if (obj)
                    retval.push_back(obj);
                else {
                    // This shouldn't ever happen, but it might when loading
                    // an invalid file.
                    QT3DS_ASSERT(false);
                }
            }
            return retval;
        }

        void SEditorImpl::Set(const Option<SValue> &inData, STransition *&inTarget)
        {
            inTarget = NULL;
            if (inData.hasValue()) {
                TObjPtr theData(inData->getData<TObjPtr>());
                inTarget = FromEditor<STransition>(theData);
            }
        }

        Option<SValue> SEditorImpl::Get(STransition *&inTarget)
        {
            return SValue(ToEditor(inTarget));
        }

        void SEditorImpl::SetInitial(const TObjList &inList, STransition *&outTransitionPtr)
        {
            if (outTransitionPtr) {
                Set(SValue(inList), outTransitionPtr->m_Target);
                if (outTransitionPtr->m_Target.mSize == 0
                    && outTransitionPtr->m_ExecutableContent.empty())
                    outTransitionPtr = NULL;
            } else {
                NVConstDataRef<SStateNode *> theTemp;
                Set(SValue(inList), theTemp);
                if (theTemp.mSize) {
                    outTransitionPtr = QT3DS_NEW(m_AutoAllocator, STransition)();
                    outTransitionPtr->m_Target = theTemp;
                }
            }
        }

        SValue SEditorImpl::GetInitial(STransition *inInitialTrans)
        {
            if (inInitialTrans)
                return Get(inInitialTrans->m_Target);

            return TObjList();
        }
        static inline STransition **GetInitialTransitionPtr(SStateNode &inParent)
        {
            StateNodeTypes::Enum typeEnum(inParent.m_Type);
            STransition **theParentTransPtr = NULL;
            if (typeEnum == StateNodeTypes::SCXML)
                theParentTransPtr = &static_cast<SSCXML &>(inParent).m_Initial;
            else if (typeEnum == StateNodeTypes::State)
                theParentTransPtr = &static_cast<SState &>(inParent).m_Initial;
            return theParentTransPtr;
        }

        static inline bool IsFirstChild(SStateNode &inChild, SStateNode &inParent)
        {
            TStateNodeList *childList = inParent.GetChildren();
            SStateNode *firstViableChild = NULL;
            if (childList) {
                for (TStateNodeList::iterator iter = childList->begin(), end = childList->end();
                     iter != end && firstViableChild == NULL; ++iter) {
                    StateNodeTypes::Enum typeEnum = iter->m_Type;
                    if (typeEnum == StateNodeTypes::State || typeEnum == StateNodeTypes::Parallel
                        || typeEnum == StateNodeTypes::Final) {
                        firstViableChild = &(*iter);
                    }
                }
            }
            return firstViableChild == &inChild;
        }

        struct SInitialTargetChange : public IChange
        {
            TObjPtr m_EditorObj;
            SStateNode &m_Parent;
            STransition *m_InitialTransition;
            STransition *m_FinalTransition;
            QT3DSI32 m_RefCount;
            // Implicitly setting c to be the initial target.
            SInitialTargetChange(TObjPtr editorObj, SStateNode &p, SStateNode &c,
                                 SEditorImpl &editor)
                : m_EditorObj(editorObj)
                , m_Parent(p)
                , m_InitialTransition(NULL)
                , m_FinalTransition(NULL)
                , m_RefCount(0)
            {
                STransition **theTransitionPtr = GetInitialTransitionPtr(m_Parent);
                QT3DS_ASSERT(theTransitionPtr);
                if (theTransitionPtr) {
                    m_InitialTransition = *theTransitionPtr;
                    theTransitionPtr[0] = NULL;
                    bool firstChild = IsFirstChild(c, p);
                    if (!firstChild) {
                        TObjPtr editorObj = editor.Create("transition", "");
                        TObjList targets;
                        targets.push_back(editor.ToEditor(c));
                        editorObj->SetPropertyValue("target", targets);
                        SStateNode *stateNode = editor.StateNodeFromEditor(editorObj);
                        m_FinalTransition = static_cast<STransition *>(stateNode);
                        m_FinalTransition->m_Parent = &p;
                        *theTransitionPtr = m_FinalTransition;
                    }
                }
            }
            virtual void addRef() { ++m_RefCount; }
            virtual void release()
            {
                --m_RefCount;
                if (m_RefCount <= 0)
                    delete this;
            }
            virtual TObjPtr GetEditor() { return m_EditorObj; }

            template <typename TNodeType>
            void Apply(STransition *trans, TNodeType &node)
            {
                node.m_Initial = trans;
            }

            virtual void Do()
            {
                if (m_Parent.m_Type == StateNodeTypes::SCXML)
                    Apply(m_FinalTransition, static_cast<SSCXML &>(m_Parent));
                else if (m_Parent.m_Type == StateNodeTypes::State)
                    Apply(m_FinalTransition, static_cast<SState &>(m_Parent));
            }

            virtual void Undo()
            {
                if (m_Parent.m_Type == StateNodeTypes::SCXML)
                    Apply(m_InitialTransition, static_cast<SSCXML &>(m_Parent));
                else if (m_Parent.m_Type == StateNodeTypes::State)
                    Apply(m_InitialTransition, static_cast<SState &>(m_Parent));
            }
        };

        void SEditorImpl::SetInitialTarget(const Option<SValue> &inData, SStateNode &inNode)
        {
            if (inNode.m_Type == StateNodeTypes::Transition
                /**|| inNode.m_Type == StateNodeTypes::History **/) {
                QT3DS_ASSERT(false);
            }
            SStateNode *theParent = inNode.m_Parent;
            if (theParent) {
                STransition **theParentTransPtr = GetInitialTransitionPtr(*theParent);

                if (theParentTransPtr) {
                    bool newValue = false;
                    if (inData.hasValue())
                        newValue = inData->getData<bool>();

                    NVScopedRefCounted<SInitialTargetChange> theChange =
                        new SInitialTargetChange(ToEditor(*theParent), *theParent, inNode, *this);
                    theChange->Do();
                    STransaction *theTransaction = GetOpenTransactionImpl();
                    if (theTransaction) {
                        theTransaction->m_Changes.push_back(theChange.mPtr);
                        TStateNodeList *children = theParent->GetChildren();
                        for (TStateNodeList::iterator iter = children->begin(),
                                                      end = children->end();
                             iter != end; ++iter) {
                            theTransaction->m_Changes.push_back(new SChange(*ToEditor(*iter)));
                        }
                    }
                }
            }
        }

        SValue SEditorImpl::IsInitialTarget(SStateNode &inNode)
        {
            if (inNode.m_Type == StateNodeTypes::Transition
                /** || inNode.m_Type == StateNodeTypes::History **/)
                return SValue(false);

            SStateNode *theParent(inNode.m_Parent);
            if (theParent) {
                if (!isTrivial(theParent->GetInitialExpression()))
                    return false;

                STransition *theTransition = theParent->GetInitialTransition();
                if (theTransition) {
                    for (QT3DSU32 idx = 0, end = theTransition->m_Target.size(); idx < end; ++idx)
                        if (&inNode == theTransition->m_Target[idx])
                            return SValue(true);
                } else {
                    if (GetInitialTransitionPtr(*theParent))
                        return SValue(IsFirstChild(inNode, *theParent));
                }
            }
            return SValue(false);
        }

        TEditorStr SEditorImpl::GetDefaultInitialValue(SStateNode &inNode)
        {
            TStateNodeList *children = inNode.GetChildren();
            if (children == NULL)
                return TEditorStr();
            for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                 iter != end; ++iter) {
                if (iter->m_Type != StateNodeTypes::History
                    && iter->m_Type != StateNodeTypes::Transition)
                    return TEditorStr(iter->m_Id.c_str());
            }

            for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                 iter != end; ++iter) {
                if (iter->m_Type != StateNodeTypes::Transition)
                    return TEditorStr(iter->m_Id.c_str());
            }

            return TEditorStr();
        }

        void SEditorImpl::GetLegalInitialValues(SStateNode &inNode,
                                                eastl::vector<CRegisteredString> &outValues)
        {
            TStateNodeList *children = inNode.GetChildren();
            if (children == NULL)
                return;
            for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                 iter != end; ++iter) {
                if (/** iter->m_Type != StateNodeTypes::History && **/ iter->m_Type
                    != StateNodeTypes::Transition)
                    outValues.push_back(iter->m_Id);
            }
        }

        struct SInitialComboChange : public IChange
        {
            SStateNode &m_Node;
            TObjPtr m_EditorObj;
            const TEditorStr m_PreEditorComboValue;
            const TEditorStr m_PostEditorComboValue;
            const char8_t *m_PreInitialExpression;
            const char8_t *m_PostInitialExpression;
            STransition *m_PreComboTransition;
            STransition *m_PostComboTransition;
            QT3DSI32 m_RefCount;
            SInitialComboChange(SStateNode &node, TObjPtr editObj, const char8_t *preExpr,
                                const char8_t *postExpr, STransition *preCombo,
                                STransition *postCombo, const TEditorStr &preEditorComboValue,
                                const TEditorStr &postEditorComboValue)
                : m_Node(node)
                , m_EditorObj(editObj)
                , m_RefCount(0)
                , m_PreInitialExpression(preExpr)
                , m_PostInitialExpression(postExpr)
                , m_PreComboTransition(preCombo)
                , m_PostComboTransition(postCombo)
                , m_PreEditorComboValue(preEditorComboValue)
                , m_PostEditorComboValue(postEditorComboValue)
            {
            }

            virtual void addRef() { ++m_RefCount; }
            virtual void release()
            {
                --m_RefCount;
                if (m_RefCount <= 0)
                    delete this;
            }
            virtual TObjPtr GetEditor() { return m_EditorObj; }

            template <typename TNodeType, typename TEditorType>
            void Apply(const char8_t *expr, STransition *trans, const TEditorStr &comboValue,
                       TNodeType &node, TEditorType &editor)
            {
                node.m_Initial = trans;
                node.m_InitialExpr = expr;
                editor.ApplyInitial(comboValue);
                // editor.m_InitialComboValue = comboValue;
            }

            virtual void Do()
            {
                if (m_Node.m_Type == StateNodeTypes::SCXML)
                    Apply(m_PostInitialExpression, m_PostComboTransition, m_PostEditorComboValue,
                          static_cast<SSCXML &>(m_Node), static_cast<SSCXMLEditor &>(*m_EditorObj));
                else if (m_Node.m_Type == StateNodeTypes::State)
                    Apply(m_PostInitialExpression, m_PostComboTransition, m_PostEditorComboValue,
                          static_cast<SState &>(m_Node), static_cast<SStateEditor &>(*m_EditorObj));
            }

            virtual void Undo()
            {
                if (m_Node.m_Type == StateNodeTypes::SCXML)
                    Apply(m_PreInitialExpression, m_PreComboTransition, m_PreEditorComboValue,
                          static_cast<SSCXML &>(m_Node), static_cast<SSCXMLEditor &>(*m_EditorObj));
                else if (m_Node.m_Type == StateNodeTypes::State)
                    Apply(m_PreInitialExpression, m_PreComboTransition, m_PreEditorComboValue,
                          static_cast<SState &>(m_Node), static_cast<SStateEditor &>(*m_EditorObj));
            }
        };

        void SEditorImpl::SetInitialAttribute(const Option<SValue> &inData, SStateNode &inNode)
        {
            TObjPtr theEditor = ToEditor(inNode);
            STransition **targetPtr = NULL;
            const char8_t **exprPtr = NULL;
            TEditorStr comboValue;
            if (inNode.m_Type == StateNodeTypes::State) {
                targetPtr = &static_cast<SState &>(inNode).m_Initial;
                exprPtr = &static_cast<SState &>(inNode).m_InitialExpr;
                comboValue = static_cast<SStateEditor &>(*theEditor).m_InitialComboValue;
            } else if (inNode.m_Type == StateNodeTypes::SCXML) {
                targetPtr = &static_cast<SSCXML &>(inNode).m_Initial;
                exprPtr = &static_cast<SSCXML &>(inNode).m_InitialExpr;
                comboValue = static_cast<SSCXMLEditor &>(*theEditor).m_InitialComboValue;
            }

            if (targetPtr == NULL) {
                QT3DS_ASSERT(false);
                return;
            }

            STransition *initialTrans = *targetPtr;
            const char8_t *initialExpr = *exprPtr;
            TEditorStr initialComboValue = comboValue;

            if (inData.isEmpty()) {
                *targetPtr = NULL;
                comboValue = "(script expression)";
            } else {
                TEditorStr stateId = inData->getData<TEditorStr>();
                SStateNode *theNode =
                    this->m_StateContext->FindStateNode(RegisterStr(stateId.c_str()));
                if (theNode) {
                    *targetPtr = QT3DS_NEW(this->m_AutoAllocator, STransition)();
                    SStateNode **newNode =
                        reinterpret_cast<SStateNode **>(this->m_AutoAllocator.allocate(
                            sizeof(SStateNode *), "TargetList", __FILE__, __LINE__));
                    targetPtr[0]->m_Target = toDataRef(newNode, 1);
                    *const_cast<SStateNode **>(targetPtr[0]->m_Target.begin()) = theNode;
                    targetPtr[0]->m_Target.mSize = 1;
                    comboValue = stateId;
                } else {
                    *targetPtr = NULL;
                    comboValue = "";
                }
                *exprPtr = "";
            }
            STransition *postTrans = *targetPtr;
            const char8_t *postExpr = *exprPtr;
            TEditorStr postComboValue = comboValue;
            STransaction *theTransaction = GetOpenTransactionImpl();
            if (theTransaction) {
                SInitialComboChange *theChange = new SInitialComboChange(
                    inNode, ToEditor(inNode), initialExpr, postExpr, initialTrans, postTrans,
                    initialComboValue, postComboValue);
                theChange->Do();
                theTransaction->m_Changes.push_back(theChange);
                TStateNodeList *children = inNode.GetChildren();
                if (children) {
                    for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                         iter != end; ++iter) {
                        theTransaction->m_Changes.push_back(new SChange(*ToEditor(*iter)));
                    }
                }
            }
        }

        void SEditorImpl::Set(const Option<SValue> &inData, SDataModel *&inTarget)
        {
            if (inData.hasValue()) {
                TObjPtr thePtr = inData->getData<TObjPtr>();
                if (thePtr)
                    inTarget = FromEditor<SDataModel>(thePtr);
                else
                    inTarget = NULL;
            } else
                inTarget = NULL;
        }

        SValue SEditorImpl::Get(SDataModel *inTarget) { return ToEditor(inTarget); }

        void SEditorImpl::Set(const Option<SValue> &inData, TDataList &inTarget)
        {
            while (inTarget.empty() == false)
                inTarget.remove(inTarget.front());
            if (inData.hasValue()) {
                TObjList newData = inData->getData<TObjList>();
                for (TObjList::iterator iter = newData.begin(), end = newData.end(); iter != end;
                     ++iter) {
                    SData *entry = FromEditor<SData>(*iter);
                    if (entry)
                        inTarget.push_back(*entry);
                }
            }
        }

        SValue SEditorImpl::Get(TDataList &inTarget)
        {
            TObjList retval;
            for (TDataList::iterator iter = inTarget.begin(), end = inTarget.end(); iter != end;
                 ++iter) {
                TObjPtr item = ToEditor(*iter);
                if (item)
                    retval.push_back(item);
            }
            return retval;
        }

        void SEditorImpl::Set(const Option<SValue> &inData, NVConstDataRef<QT3DSVec2> &inTarget)
        {
            if (inTarget.size()) {
                m_AutoAllocator.deallocate((void *)inTarget.mData);
                inTarget.mData = NULL;
                inTarget.mSize = 0;
            }
            if (inData.hasValue()) {
                TVec2List newData(inData->getData<TVec2List>());
                if (newData.size()) {
                    size_t newDataSize = newData.size() * sizeof(QT3DSVec2);
                    QT3DSVec2 *newTargetData = (QT3DSVec2 *)m_AutoAllocator.allocate(
                        newDataSize, "Transition::m_Path", __FILE__, __LINE__);
                    memCopy(newTargetData, newData.data(), newDataSize);
                    inTarget = toDataRef(newTargetData, newData.size());
                }
            }
        }

        SValue SEditorImpl::Get(const NVConstDataRef<QT3DSVec2> &inTarget)
        {
            TVec2List retval;
            if (inTarget.mSize)
                retval.insert(retval.end(), inTarget.begin(), inTarget.end());
            return retval;
        }

        void SEditorImpl::Set(const Option<SValue> &inData, SSend *&inTarget)
        {
            inTarget = NULL;
            if (inData.hasValue() && inData->getType() == ValueTypes::ObjPtr) {
                TObjPtr object = inData->getData<TObjPtr>();
                SSend *newPtr = FromEditor<SSend>(object);
                inTarget = newPtr;
            }
        }

        SValue SEditorImpl::Get(SSend *inTarget)
        {
            if (!inTarget)
                return SValue(TObjPtr());
            return ToEditor(inTarget);
        }

        TObjPtr SEditorImpl::CreateExecutableContent(SStateNode &inParent,
                                                     const char8_t *inTypeName)
        {
            SExecutableContent *newExecutable = NULL;
            TObjPtr newObj;
            if (AreEqual(inTypeName, SSendEditor::GetTypeStr())) {
                eastl::pair<SSend *, TObjPtr> theNewEditor = CreateEditorAndObject<SSend>();
                GenerateUniqueId(*theNewEditor.first, "send");
                newExecutable = theNewEditor.first;
                newObj = theNewEditor.second;
            } else if (AreEqual(inTypeName, SScriptEditor::GetTypeStr())) {
                eastl::pair<SScript *, TObjPtr> theNewEditor = CreateEditorAndObject<SScript>();
                newExecutable = theNewEditor.first;
                newObj = theNewEditor.second;
            } else if (AreEqual(inTypeName, SCancelEditor::GetTypeStr())) {
                eastl::pair<SCancel *, TObjPtr> theNewEditor = CreateEditorAndObject<SCancel>();
                newExecutable = theNewEditor.first;
                newObj = theNewEditor.second;
            } else {
                QT3DS_ASSERT(false);
                return TObjPtr();
            }
            if (newExecutable)
                newExecutable->m_StateNodeParent = &inParent;
            return newObj;
        }

        SValue SEditorImpl::GetSendId(const char8_t *expression)
        {
            if (isTrivial(expression))
                return SValue((QT3DSU32)0);
            return static_cast<QT3DSU32>(IExecutionContext::ParseTimeStrToMilliseconds(expression));
        }

        void SEditorImpl::SetSendId(const Option<SValue> &inData, const char8_t *&outExpression)
        {
            if (!isTrivial(outExpression))
                m_AutoAllocator.deallocate(const_cast<char8_t *>(outExpression));

            outExpression = "";

            if (inData.hasValue() && inData->getType() == ValueTypes::U32) {
                QT3DSU32 expr = inData->getData<QT3DSU32>();
                char buffer[64];
                sprintf(buffer, "%u", expr);
                int len = strlen(buffer);
                if (len < 61) {
                    buffer[len] = 'm';
                    buffer[len + 1] = 's';
                    buffer[len + 2] = 0;
                }
                outExpression = ToGraphStr(buffer);
            }
        }

        IStateContext &SEditorImpl::GetStateContext() { return *m_StateContext; }

        void SEditorImpl::SetTransactionManager(STransactionManagerImpl &manager)
        {
            m_TransactionManager = &manager;
        }

        bool IsLegalHistoryTarget(StateNodeTypes::Enum inType)
        {
            return inType == StateNodeTypes::State || inType == StateNodeTypes::Final
                || inType == StateNodeTypes::Parallel;
        }

        void RecurseGetChildrenForHistoryTarget(SState &inNode,
                                                eastl::vector<CRegisteredString> &retval)
        {
            for (TStateNodeList::iterator iter = inNode.m_Children.begin(),
                                          end = inNode.m_Children.end();
                 iter != end; ++iter) {
                if (IsLegalHistoryTarget(iter->m_Type)) {
                    retval.push_back(iter->m_Id);
                    if (iter->m_Type == StateNodeTypes::State)
                        RecurseGetChildrenForHistoryTarget(static_cast<SState &>(*iter), retval);
                }
            }
        }

        eastl::vector<CRegisteredString> SEditorImpl::GetLegalHistoryDefaultValues(SHistory &inData)
        {
            eastl::vector<CRegisteredString> retval;
            if (inData.m_Parent && inData.m_Parent->m_Type == StateNodeTypes::State) {
                SState &theState = static_cast<SState &>(*inData.m_Parent);
                for (TStateNodeList::iterator iter = theState.m_Children.begin(),
                                              end = theState.m_Children.end();
                     iter != end; ++iter) {
                    if (IsLegalHistoryTarget(iter->m_Type)) {
                        retval.push_back(iter->m_Id);
                        if (inData.m_Flags.IsDeep()) {
                            if (iter->m_Type == StateNodeTypes::State)
                                RecurseGetChildrenForHistoryTarget(static_cast<SState &>(*iter),
                                                                   retval);
                        }
                    }
                }
            }
            return retval;
        }

        void RecurseGetLegalParents(eastl::vector<CRegisteredString> &outResult,
                                    SStateNode &inTarget, SStateNode &inCurrent)
        {
            // Basic checks
            if (inCurrent.m_Type == StateNodeTypes::History
                || inCurrent.m_Type == StateNodeTypes::Final || &inCurrent == &inTarget)
                return;

            // Parallel's cannot have finals, but their children could
            bool isIllegalChild = (inTarget.m_Type == StateNodeTypes::Final
                                   && inCurrent.m_Type == StateNodeTypes::Parallel)
                || inCurrent.m_Type == StateNodeTypes::SCXML;

            if (isIllegalChild == false) {
                outResult.push_back(inCurrent.m_Id);
            }
            TStateNodeList *children = inCurrent.GetChildren();
            if (children) {
                for (TStateNodeList::iterator iter = children->begin(), end = children->end();
                     iter != end; ++iter) {
                    RecurseGetLegalParents(outResult, inTarget, *iter);
                }
            }
        }

        static bool lexi(CRegisteredString lhs, CRegisteredString rhs)
        {
            return strcmp(lhs.c_str(), rhs.c_str()) < 0;
        }

        eastl::vector<CRegisteredString> SEditorImpl::GetLegalParentIds(SStateNode &inNode)
        {
            // Do a depth first search and just do not include this node or any history or final
            // nodes.
            eastl::vector<CRegisteredString> retval;
            RecurseGetLegalParents(retval, inNode, *m_StateContext->GetRoot());
            eastl::sort(retval.begin(), retval.end(), lexi);
            return retval;
        }

        struct SParentChange : public IChange
        {
            TObjPtr m_ChildObj;
            TStateNodeList *m_OldList;
            TStateNodeList &m_NewList;
            SStateNode *m_OldParent;
            SStateNode &m_NewParent;
            SStateNode &m_Child;
            QT3DSI32 m_RefCount;
            SParentChange(TObjPtr childObj, TStateNodeList *oldL, TStateNodeList &newL,
                          SStateNode *oldp, SStateNode &newp, SStateNode &c)
                : m_ChildObj(childObj)
                , m_OldList(oldL)
                , m_NewList(newL)
                , m_OldParent(oldp)
                , m_NewParent(newp)
                , m_Child(c)
                , m_RefCount(0)
            {
            }

            void addRef() { ++m_RefCount; }
            void release()
            {
                --m_RefCount;
                if (m_RefCount <= 0)
                    delete this;
            }

            virtual void Do()
            {
                if (m_OldList)
                    m_OldList->remove(m_Child);
                m_NewList.push_back(m_Child);
                m_Child.m_Parent = &m_NewParent;
            }
            virtual void Undo()
            {
                m_NewList.remove(m_Child);
                if (m_OldList)
                    m_OldList->push_back(m_Child);
                m_Child.m_Parent = m_OldParent;
            }

            virtual TObjPtr GetEditor() { return m_ChildObj; }
        };

        void SEditorImpl::SetParent(SStateNode &inNode, const Option<SValue> &inValue)
        {
            STransaction *theTransaction = m_TransactionManager->GetOpenTransactionImpl();
            SStateNode *oldParent = inNode.m_Parent;
            TEditorStr newParentId;
            if (inValue.hasValue())
                newParentId = inValue->getData<TEditorStr>();

            SStateNode *newParent =
                m_StateContext->FindStateNode(m_StringTable->RegisterStr(newParentId.c_str()));
            if (newParent == NULL)
                newParent = m_StateContext->GetRoot();
            TStateNodeList *newList = newParent->GetChildren();
            if (newList == NULL) {
                QT3DS_ASSERT(false);
                return;
            }
            SParentChange *theChange =
                new SParentChange(ToEditor(inNode), oldParent ? oldParent->GetChildren() : NULL,
                                  *newList, oldParent, *newParent, inNode);
            theChange->Do();
            if (theTransaction) {
                theTransaction->m_Changes.push_back(theChange);
                if (oldParent)
                    theTransaction->m_Changes.push_back(new SChange(*ToEditor(*oldParent)));
                theTransaction->m_Changes.push_back(new SChange(*ToEditor(*newParent)));
            }
            CheckAndSetValidHistoryDefault(ToEditor(inNode));
            // Fix the initial state of the old parent if it exists
            if (oldParent) {
                if (isTrivial(oldParent->GetInitialExpression())
                    && oldParent->GetInitialTransition()) {
                    STransition *theTransition = oldParent->GetInitialTransition();
                    if (theTransition != NULL && theTransition->m_Target.size()) {
                        bool foundItem = false;
                        for (QT3DSU32 idx = 0, end = theTransition->m_Target.size();
                             idx < end && foundItem == false; ++idx) {
                            if (theTransition->m_Target[idx] == &inNode)
                                foundItem = true;
                        }
                        if (foundItem) {
                            SetInitialAttribute(SValue(TEditorStr()), *oldParent);
                        }
                    }
                }
                // fix all history nodes
                TStateNodeList *parentChildren = oldParent->GetChildren();
                if (parentChildren) {
                    for (TStateNodeList::iterator iter = parentChildren->begin(),
                                                  end = parentChildren->end();
                         iter != end; ++iter) {
                        if (iter->m_Type == StateNodeTypes::History) {
                            CheckAndSetValidHistoryDefault(ToEditor(*iter));
                        }
                    }
                }
            }
            // Set the new object inside the new parent.
            ToEditor(inNode)->SetPropertyValue("position", QT3DSVec2(0, 0));
        }

        void SEditorImpl::CheckAndSetValidHistoryDefault(TObjPtr inHistoryNode)
        {
            if (inHistoryNode && AreEqual(inHistoryNode->TypeName(), "history")) {
                eastl::vector<CRegisteredString> validChildren =
                    inHistoryNode->GetLegalValues("default");
                TObjList existing = inHistoryNode->GetPropertyValue("default")->getData<TObjList>();
                if (existing.size()) {
                    // Ensure all of the existing nodes are in the valid children list.
                    bool allValid = true;
                    for (size_t existingIdx = 0, existingEnd = existing.size();
                         existingIdx < existingEnd && allValid; ++existingIdx) {
                        CRegisteredString idStr =
                            m_StringTable->RegisterStr(existing[existingIdx]->GetId().c_str());
                        if (eastl::find(validChildren.begin(), validChildren.end(), idStr)
                            == validChildren.end())
                            allValid = false;
                    }
                    if (allValid)
                        return;
                }
                SetValidHistoryDefault(inHistoryNode);
            }
        }

        void SEditorImpl::SetValidHistoryDefault(TObjPtr inHistoryNode)
        {
            if (inHistoryNode && AreEqual(inHistoryNode->TypeName(), "history")) {
                eastl::vector<CRegisteredString> legalValues =
                    inHistoryNode->GetLegalValues("default");
                if (legalValues.size()) {
                    TObjPtr firstLegal = GetObjectById(legalValues[0]);
                    TObjList propValue;
                    propValue.push_back(firstLegal);
                    inHistoryNode->SetPropertyValue("default", propValue);
                }
            }
        }

        MallocAllocator SBaseEditorFoundation::g_BaseAlloc;

        bool SPropertyDecNameFinder::operator()(const SPropertyDeclaration &dec) const
        {
            return AreEqual(m_NameStr, dec.m_Name.c_str());
        }

        void IEditorObject::Append(const char8_t *inPropName, TObjPtr inObj)
        {
            Option<SValue> theProp = GetPropertyValue(inPropName);
            if (theProp.hasValue() && theProp->getType() == ValueTypes::ObjPtrList
                && inObj != NULL) {
                TObjList theData = theProp->getData<TObjList>();
                theData.push_back(inObj);
                SetPropertyValue(inPropName, theData);
            }
        }

        void IEditorObject::Remove(const char8_t *inPropName, TObjPtr inObj)
        {
            Option<SValue> theProp = GetPropertyValue(inPropName);
            if (theProp.hasValue() && theProp->getType() == ValueTypes::ObjPtrList
                && inObj != NULL) {
                TObjList theData = theProp->getData<TObjList>();
                TObjList::iterator theFind = eastl::find(theData.begin(), theData.end(), inObj);
                if (theFind != theData.end())
                    theData.erase(theFind);
                SetPropertyValue(inPropName, theData);
            }
        }

        IEditor &IEditor::CreateEditor()
        {
            TFoundationPtr theFoundation = SBaseEditorFoundation::Create();
            return *QT3DS_NEW(theFoundation->getAllocator(), SEditorImpl)(
                theFoundation, IStringTable::CreateStringTable(theFoundation->getAllocator()));
        }

        IEditor *IEditor::CreateEditor(const char8_t *inFname)
        {
            CFileSeekableIOStream theFile(inFname, FileReadFlags());
            if (theFile.IsOpen() == false)
                return NULL;

            return CreateEditor(inFname, theFile);
        }

        IEditor *IEditor::CreateEditor(const char8_t *inFname, IInStream &inStream)
        {
            IEditor &theEditor(CreateEditor());
            if (static_cast<SEditorImpl &>(theEditor).Load(inStream, inFname))
                return &theEditor;

            theEditor.release();
            return NULL;
        }
    }
}
}
