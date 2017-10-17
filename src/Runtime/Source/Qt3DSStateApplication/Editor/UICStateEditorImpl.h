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
#ifndef QT3DS_STATE_EDITOR_IMPL_H
#define QT3DS_STATE_EDITOR_IMPL_H
#include "UICStateEditor.h"
#include "UICStateEditorValue.h"
#include "UICStateTypes.h"
#include "UICStateEditorFoundation.h"
#include "foundation/Utils.h"
#include "foundation/XML.h"
#include "UICStateXMLIO.h"
#include "foundation/IOStreams.h"
#include "UICStateExecutionTypes.h"
#include "UICStateContext.h"

namespace uic {
namespace state {
    namespace editor {

        struct SEditorImpl;

        typedef nvhash_map<void *, TObjPtr> TStateEditorMap;

        class IPropertyAccessor : public NVRefCounted
        {
        protected:
            virtual ~IPropertyAccessor() {}
        public:
            SPropertyDeclaration m_Declaration;
            IPropertyAccessor(const SPropertyDeclaration &inDec)
                : m_Declaration(inDec)
            {
            }
            virtual eastl::vector<CRegisteredString> GetLegalValues(IEditorObject & /*inObj*/)
            {
                return eastl::vector<CRegisteredString>();
            }

            virtual Option<SValue> Get(IEditorObject &inObj) = 0;
            virtual void Set(IEditorObject &inObj, const Option<SValue> &inValue) = 0;
            // Return true if this access handles the transaction code itself.
            // Currently only used for the is initial target property.
            virtual bool HandlesTransaction() { return false; }
        };

        typedef NVScopedRefCounted<IPropertyAccessor> TPropertyAccessorPtr;
        typedef eastl::vector<TPropertyAccessorPtr> TPropertyAccessorList;
        typedef nvhash_map<int, TPropertyAccessorList> TAccessorMap;

#define QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(foundationPtr, refcountVar)                   \
    void addRef() { atomicIncrement(&(refcountVar)); }                                             \
    void release()                                                                                 \
    {                                                                                              \
        TFoundationPtr temp(foundationPtr);                                                        \
        QT3DSI32 value = atomicDecrement(&(refcountVar));                                             \
        if (value <= 0)                                                                            \
            NVDelete(foundationPtr->getAllocator(), this);                                         \
    }

        struct SEditorImplStrIOStream : public IOutStream
        {
            TEditorStr m_Str;
            bool Write(NVConstDataRef<QT3DSU8> data)
            {
                if (data.size())
                    m_Str.append((const char8_t *)data.begin(), (const char8_t *)data.end());
                return true;
            }
        };

        struct SEditorImplStrInStream : public IInStream
        {
            const char8_t *m_Pos;
            const char8_t *m_End;
            SEditorImplStrInStream(const TEditorStr &inStr)
                : m_Pos(inStr.data())
                , m_End(inStr.data() + inStr.size())
            {
            }

            virtual QT3DSU32 Read(NVDataRef<QT3DSU8> data)
            {
                QT3DSU32 amountLeft = (QT3DSU32)(m_End - m_Pos);
                QT3DSU32 amountToRead = NVMin(amountLeft, data.size());
                memCopy(data.mData, m_Pos, amountToRead);
                m_Pos += amountToRead;
                return amountToRead;
            }
        };

        struct STransaction;

        typedef eastl::vector<IEditorChangeListener *> TChangeListenerList;
        struct STransactionManagerImpl;

        class IEditorCopyPasteListener
        {
        protected:
            virtual ~IEditorCopyPasteListener() {}

        public:
            virtual void OnCopy(TEditorPtr inEditor, eastl::vector<SStateNode *> &ioCopiedRoots,
                                IDOMWriter &ioWriter,
                                eastl::vector<SNamespacePair> &ioNamespaces) = 0;
            virtual void OnPaste(TEditorPtr inEditor, IDOMReader &ioReader,
                                 CXMLIO::TIdRemapMap &inStateIdRemapMap) = 0;
            virtual void OnIDChange(TEditorPtr inEditor, SStateNode &inNode,
                                    const char8_t *inOldId) = 0;
        };

        // Forward declaration of the editor interface that the properties and the editor objects
        // can all use.  Implementation in UICStateEditor.cpp
        struct SEditorImpl : public IEditor
        {
            // Note that *we* keep references to our editors
            // but our editor objects *cannot* keep a reference to us.
            // This means that our editor objects need to be able to function (not crash) if we
            // ourselves go out of scope.
            TFoundationPtr m_EditorFoundation;
            NVScopedRefCounted<IStringTable> m_StringTable;
            SSAutoDeallocatorAllocator m_AutoAllocator;
            NVScopedRefCounted<IStateContext> m_StateContext;
            TStateEditorMap m_Editors;
            volatile QT3DSI32 mRefCount;
            NVScopedRefCounted<STransactionManagerImpl> m_TransactionManager;
            TAccessorMap m_Accessors;
            IEditorCopyPasteListener *m_CopyPasteListener;

            SEditorImpl(TFoundationPtr inFoundation,
                        NVScopedRefCounted<IStringTable> inStringTable);
            void addRef();
            void release();

            TObjPtr InsertEditor(void *inData, IEditorObject *inEditor);

            template <typename TStateType>
            TObjPtr ToEditor(TStateType &inItem);

            template <typename TStateType>
            TObjPtr ToEditor(TStateType *inItem);
            TObjPtr ToEditor(SStateNode &inItem);
            TObjPtr ExecutableContentToEditor(SExecutableContent &inItem);

            template <typename TStateType>
            TStateType *FromEditor(TObjPtr inPtr);

            SStateNode *StateNodeFromEditor(TObjPtr inPtr);

            SExecutableContent *ExecutableContentFromEditor(TObjPtr inPtr);
            void GenerateUniqueId(SStateNode &inNode, const char8_t *inStem);
            void GenerateUniqueId(SSend &inNode, const char8_t *inStem);

            virtual TObjPtr GetRoot();

            template <typename TStateType>
            eastl::pair<TStateType *, TObjPtr> CreateEditorAndObject();

            TObjPtr DoCreate(const char8_t *inTypeName, const char8_t *inId);

            virtual TObjPtr Create(const char8_t *inTypeName, const char8_t *inId);

            virtual TObjPtr GetOrCreate(SSCXML &inData);
            virtual TObjPtr GetOrCreate(SState &inData);
            virtual TObjPtr GetOrCreate(STransition &inData);
            virtual TObjPtr GetOrCreate(SParallel &inData);
            virtual TObjPtr GetOrCreate(SFinal &inData);
            virtual TObjPtr GetOrCreate(SHistory &inData);
            virtual TObjPtr GetOrCreate(SDataModel &inData);
            virtual TObjPtr GetOrCreate(SData &inData);

            virtual TObjPtr GetObjectById(const char8_t *inId);

            virtual TObjPtr GetEditor(void *inGraphData);

            virtual TSignalConnectionPtr AddChangeListener(IEditorChangeListener &inListener);

            void RemoveChangeListener(IEditorChangeListener &inListener);

            virtual TTransactionPtr BeginTransaction(const TEditorStr &inName);

            virtual TTransactionPtr GetOpenTransaction();

            STransaction *GetOpenTransactionImpl();

            virtual void RollbackTransaction();

            virtual void EndTransaction();

            virtual TEditorStr Copy(TObjList inObjects, const QT3DSVec2 &inMousePos);

            virtual bool CanPaste(TObjPtr inTarget);

            void AddNewPasteObjectToTransaction(SStateNode &inNode);

            virtual void Paste(const TEditorStr &inCopiedObjects, TObjPtr inTarget,
                               const QT3DSVec2 &inMousePos);

            static bool IsDerivedFrom(SStateNode &child, SStateNode &parent);

            // This method should always return a value because the scxml root item
            // is the parent of everyone else.
            static SStateNode &GetLeastCommonAncestor(SStateNode &lhs, SStateNode &rhs);

            TObjPtr GetLeastCommonAncestor(NVConstDataRef<TObjPtr> inObjects);

            TObjList GetCancelableSendIds();

            TObjPtr ChangeExecutableContentType(TObjPtr inContent, const char8_t *newType);

            TEditorStr ToXML(TObjPtr inContent);

            eastl::pair<TEditorStr, TObjPtr> FromXML(TObjPtr inContent,
                                                     const TEditorStr &ioEditedXML);

            // Return the set of events in use in the state machine.
            TEditorStrList GetStateMachineEvents();

            void ReleaseEditor(void *inData);

            virtual bool Save(const char8_t *inFileName);
            virtual void Save(IOutStream &inStream);

            bool Load(IInStream &inStream, const char8_t *inFilename);

            /////////////////////////////////////////////////////////////////////
            // Property access helpers
            /////////////////////////////////////////////////////////////////////
            void SetIdProperty(SStateNode &inNode, const SValue &inData,
                               CRegisteredString &inTarget);
            void SetIdProperty(SSend &inNode, const SValue &inData, CRegisteredString &inTarget);

            void Set(const SValue &inData, CRegisteredString &inTarget)
            {
                TEditorStr theStr(inData.getData<TEditorStr>());
                inTarget = m_StringTable->RegisterStr(theStr.c_str());
            }

            void Set(const SValue &inData, const char8_t *&inTarget)
            {
                TEditorStr theStr(inData.getData<TEditorStr>());
                const char8_t *theData(theStr.c_str());

                if (inTarget && *inTarget != 0)
                    m_AutoAllocator.deallocate(const_cast<char8_t *>(inTarget));

                size_t len = StrLen(theData);
                if (len == 0)
                    inTarget = NULL;

                ++len; // account for null terminate

                char8_t *newTarget = (char8_t *)m_AutoAllocator.allocate(len + 1, "graph string",
                                                                         __FILE__, __LINE__);
                memCopy(newTarget, theData, len);
                inTarget = newTarget;
            }
            SValue Get(CRegisteredString &inTarget) { return inTarget.c_str(); }
            SValue Get(const char8_t *inItem) { return nonNull(inItem); }

            void Set(const Option<SValue> &inData, NVConstDataRef<SStateNode *> &inTarget);
            SValue Get(NVConstDataRef<SStateNode *> &inTarget);

            void Set(const Option<SValue> &inData, STransition *&inTarget);
            Option<SValue> Get(STransition *&inTarget);

            void SetInitial(const TObjList &inList, STransition *&outInitialTransition);
            SValue GetInitial(STransition *inInitialTransition);

            void SetInitialTarget(const Option<SValue> &inData, SStateNode &inNode);
            SValue IsInitialTarget(SStateNode &inNode);

            void Set(const Option<SValue> &inData, SDataModel *&inTarget);
            SValue Get(SDataModel *inTarget);

            void Set(const Option<SValue> &inData, TDataList &inTarget);
            SValue Get(TDataList &inTarget);

            void Set(const Option<SValue> &inData, NVConstDataRef<QT3DSVec2> &inTarget);
            SValue Get(const NVConstDataRef<QT3DSVec2> &inTarget);

            void Set(const Option<SValue> &inData, SSend *&inTarget);
            SValue Get(SSend *inTarget);

            void Set(const Option<SValue> &inData, TEditorStr &inTarget)
            {
                inTarget.clear();
                if (inData.hasValue())
                    inTarget = inData->getData<TEditorStr>();
            }

            SValue Get(const TEditorStr &inTarget) { return inTarget; }

            void Set(const Option<SValue> &inData, Option<QT3DSVec2> &inTarget)
            {
                if (inData.hasValue()) {
                    inTarget = inData->getData<QT3DSVec2>();
                } else {
                    inTarget = Empty();
                }
            }
            Option<SValue> Get(const Option<QT3DSVec2> &inTarget)
            {
                Option<SValue> retval;
                if (inTarget.hasValue())
                    retval = SValue(inTarget.getValue());
                return retval;
            }

            void Set(const Option<SValue> &inData, TVec2List &inTarget)
            {
                if (inData.hasValue()) {
                    inTarget = inData->getData<TVec2List>();
                } else {
                    inTarget.clear();
                }
            }
            SValue Get(const TVec2List &inTarget) { return inTarget; }

            void Set(const Option<SValue> &inData, Option<QT3DSVec3> &inTarget)
            {
                if (inData.hasValue()) {
                    inTarget = inData->getData<QT3DSVec3>();
                } else {
                    inTarget = Empty();
                }
            }
            const Option<SValue> Get(const Option<QT3DSVec3> &inTarget)
            {
                Option<SValue> retval;
                if (inTarget.hasValue())
                    retval = SValue(inTarget.getValue());
                return retval;
            }

            const char8_t *ToGraphStr(const char8_t *inStr)
            {
                if (isTrivial(inStr))
                    return "";
                QT3DSU32 len = StrLen(inStr) + 1;
                char8_t *retval =
                    (char8_t *)m_AutoAllocator.allocate(len, "GraphStr", __FILE__, __LINE__);
                memCopy(retval, inStr, len);
                return retval;
            }

            SValue GetSendId(const char8_t *expression);
            void SetSendId(const Option<SValue> &inData, const char8_t *&outExpression);

            TObjPtr CreateExecutableContent(SStateNode &inParent, const char8_t *inTypeName);
            eastl::pair<SExecutableContent *, TObjPtr>
            CreateExecutableContent(const char8_t *inTypeName);

            CRegisteredString RegisterStr(const char8_t *str)
            {
                return m_StringTable->RegisterStr(str);
            }
            IStateContext &GetStateContext();

            TEditorStr GetDefaultInitialValue(SStateNode &inNode);
            void GetLegalInitialValues(SStateNode &inNode,
                                       eastl::vector<CRegisteredString> &outValues);
            void SetInitialAttribute(const Option<SValue> &inData, SStateNode &inNode);

            void SetTransactionManager(STransactionManagerImpl &trans);

            void ReplaceExecutableContent(SExecutableContent &oldContent,
                                          SExecutableContent &newContent);

            eastl::vector<CRegisteredString> GetLegalHistoryDefaultValues(SHistory &inData);

            eastl::vector<CRegisteredString> GetLegalParentIds(SStateNode &inNode);
            void SetParent(SStateNode &inNode, const Option<SValue> &inValue);
            void CheckAndSetValidHistoryDefault(TObjPtr inHistoryNode);
            void SetValidHistoryDefault(TObjPtr inHistoryNode);
        };
    }
}
}
#endif