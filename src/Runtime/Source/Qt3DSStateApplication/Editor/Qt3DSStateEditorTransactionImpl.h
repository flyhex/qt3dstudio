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
#ifndef QT3DS_STATE_EDITOR_TRANSACTION_IMPL_H
#define QT3DS_STATE_EDITOR_TRANSACTION_IMPL_H
#include "Qt3DSStateEditor.h"
#include "Qt3DSStateEditorFoundation.h"
#include "Qt3DSStateEditorProperties.h"

namespace qt3ds {
namespace state {
    namespace editor {

        class IChange : public NVRefCounted
        {
        protected:
            virtual ~IChange() {}
        public:
            virtual void Do() = 0;
            virtual void Undo() = 0;
            virtual TObjPtr GetEditor() = 0;
        };

        typedef eastl::vector<NVScopedRefCounted<IChange>> TChangeList;

        struct SChange : public IChange
        {
            Option<SValue> m_OldVal;
            Option<SValue> m_NewVal;
            TPropertyAccessorPtr m_Accessor;
            TObjPtr m_Editor;
            QT3DSI32 mRefCount;

            SChange(const Option<SValue> &inOldVal, const Option<SValue> &inNewVal,
                    IPropertyAccessor &inAccessor, IEditorObject &inEditor)
                : m_OldVal(inOldVal)
                , m_NewVal(inNewVal)
                , m_Accessor(inAccessor)
                , m_Editor(inEditor)
                , mRefCount(0)
            {
            }
            // Sometimes we need to create a change just to signal a new object.
            // In this case there isn't an accessor and there aren't old values and
            // new values.
            SChange(IEditorObject &inEditor)
                : m_Editor(inEditor)
                , mRefCount(0)
            {
            }
            SChange()
                : mRefCount(0)
            {
            }
            void Do()
            {
                if (m_Accessor)
                    m_Accessor->Set(*m_Editor, m_NewVal);
            }

            void Undo()
            {
                if (m_Accessor)
                    m_Accessor->Set(*m_Editor, m_OldVal);
            }
            void addRef() { atomicIncrement(&mRefCount); }
            void release()
            {
                atomicDecrement(&mRefCount);
                if (mRefCount <= 0)
                    delete this;
            }

            TObjPtr GetEditor() { return m_Editor; }
        };

        // true if removed on do, false if removed on false;
        typedef eastl::pair<bool, TObjPtr> TRemovePair;
        typedef eastl::vector<TRemovePair> TRemovePairList;

        struct STransactionManagerImpl;

        struct STransaction : public ITransaction
        {
            TFoundationPtr m_Alloc;
            TChangeList m_Changes;
            volatile QT3DSI32 mRefCount;
            // We have to keep a refcount to the editor ourselves.
            NVScopedRefCounted<STransactionManagerImpl> m_Editor;
            TObjList m_EditorsList;
            TObjList m_RemovedEditorsList;
            TRemovePairList m_RemovedPairs;

            STransaction(TFoundationPtr alloc, STransactionManagerImpl &inEditor,
                         const TEditorStr &inName);
            virtual ~STransaction();
            virtual void addRef();
            virtual void release();

            void CreateRemovedEditorsList(bool inDo)
            {
                m_RemovedEditorsList.clear();
                for (TRemovePairList::iterator iter = m_RemovedPairs.begin(),
                                               end = m_RemovedPairs.end();
                     iter != end; ++iter)
                    if (iter->first == inDo)
                        m_RemovedEditorsList.push_back(iter->second);
            }

            struct SChangeToEditor
            {
                TObjPtr operator()(IChange *inChange) const { return inChange->GetEditor(); }
            };
            virtual void SendDoSignals();
            virtual void Do();
            virtual void SilentUndo();
            virtual void Undo();
            virtual bool Empty() { return m_Changes.empty() && m_RemovedPairs.empty(); }
            virtual TObjList GetEditedObjects()
            {
                ChangesToEditors();
                return m_EditorsList;
            }
            void ChangesToEditors();
        };

        class ITransManagerImplListener
        {
        protected:
            virtual ~ITransManagerImplListener() {}
        public:
            virtual void OnObjectDeleted(TObjPtr inObj) = 0;
            virtual void OnObjectCreated(TObjPtr inObj) = 0;
        };

        struct STransactionManagerImpl : public ITransactionManager
        {
            TFoundationPtr m_EditorFoundation;
            volatile QT3DSI32 mRefCount;
            TChangeListenerList m_ChangeListeners;
            NVScopedRefCounted<STransaction> m_Transaction;
            QT3DSI32 m_OpenCount;
            ITransManagerImplListener *m_ObjListener;

            STransactionManagerImpl(TFoundationPtr fnd)
                : m_EditorFoundation(fnd)
                , mRefCount(0)
                , m_OpenCount(0)
                , m_ObjListener(NULL)
            {
            }
            virtual void addRef();
            virtual void release();
            virtual TSignalConnectionPtr AddChangeListener(IEditorChangeListener &inListener);
            virtual TTransactionPtr BeginTransaction(const TEditorStr &inName = TEditorStr());
            virtual TTransactionPtr GetOpenTransaction();
            STransaction *GetOpenTransactionImpl();
            virtual void RollbackTransaction();
            virtual void EndTransaction();

            void OnObjectDeleted(TObjPtr inObj);
            void OnObjectCreated(TObjPtr inObj);

            void RemoveChangeListener(IEditorChangeListener &inListener);
        };
    }
}
}
#endif