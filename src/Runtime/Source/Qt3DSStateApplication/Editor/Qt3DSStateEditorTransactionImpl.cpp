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
#include "Qt3DSStateEditorTransactionImpl.h"

namespace uic {
namespace state {
    namespace editor {

        // Some things have to be implemented below so that we can take advantage of the full
        // SEditorImpl definition.

        STransaction::STransaction(TFoundationPtr alloc, STransactionManagerImpl &inEditor,
                                   const TEditorStr &inName)
            : ITransaction(inName)
            , m_Alloc(alloc)
            , mRefCount(0)
            , m_Editor(inEditor)
        {
        }

        STransaction::~STransaction() {}

        void STransaction::addRef() { atomicIncrement(&mRefCount); }

        void STransaction::release()
        {
            atomicDecrement(&mRefCount);
            if (mRefCount <= 0) {
                TFoundationPtr fnd(m_Alloc);
                NVDelete(fnd->getAllocator(), this);
            }
        }

        void STransaction::ChangesToEditors()
        {
            m_EditorsList.clear();
            m_EditorsList.resize(m_Changes.size());
            eastl::transform(m_Changes.begin(), m_Changes.end(), m_EditorsList.begin(),
                             SChangeToEditor());
        }

        void STransaction::SendDoSignals()
        {
            ChangesToEditors();
            CreateRemovedEditorsList(true);
            for (TChangeListenerList::iterator listenerIter = m_Editor->m_ChangeListeners.begin(),
                                               listenerEnd = m_Editor->m_ChangeListeners.end();
                 listenerIter != listenerEnd; ++listenerIter)
                (*listenerIter)->OnDataChange(m_EditorsList, m_RemovedEditorsList);
        }

        void STransaction::Do()
        {
            for (TChangeList::iterator iter = m_Changes.begin(), end = m_Changes.end(); iter != end;
                 ++iter)
                (*iter)->Do();

            for (TRemovePairList::iterator iter = m_RemovedPairs.begin(),
                                           end = m_RemovedPairs.end();
                 iter != end; ++iter) {
                if (iter->first)
                    iter->second->RemoveIdFromContext();
                else
                    iter->second->AddIdToContext();
            }

            SendDoSignals();
        }

        void STransaction::SilentUndo()
        {
            for (TChangeList::reverse_iterator iter = m_Changes.rbegin(), end = m_Changes.rend();
                 iter != end; ++iter)
                (*iter)->Undo();

            for (TRemovePairList::iterator iter = m_RemovedPairs.begin(),
                                           end = m_RemovedPairs.end();
                 iter != end; ++iter) {
                if (iter->first)
                    iter->second->AddIdToContext();
                else
                    iter->second->RemoveIdFromContext();
            }
        }

        void STransaction::Undo()
        {
            SilentUndo();
            m_EditorsList.clear();
            m_EditorsList.resize(m_Changes.size());
            eastl::transform(m_Changes.rbegin(), m_Changes.rend(), m_EditorsList.begin(),
                             SChangeToEditor());
            CreateRemovedEditorsList(false);
            for (TChangeListenerList::iterator listenerIter = m_Editor->m_ChangeListeners.begin(),
                                               listenerEnd = m_Editor->m_ChangeListeners.end();
                 listenerIter != listenerEnd; ++listenerIter)
                (*listenerIter)->OnDataChange(m_EditorsList, m_RemovedEditorsList);
        }

        struct STransactionImplConnection : public IStateSignalConnection
        {
            TFoundationPtr &m_Alloc;
            QT3DSI32 mRefCount;
            STransactionManagerImpl &m_Editor;
            IEditorChangeListener &m_ChangeListener;
            STransactionImplConnection(TFoundationPtr &alloc, STransactionManagerImpl &e,
                                       IEditorChangeListener &listener)
                : m_Alloc(alloc)
                , mRefCount(0)
                , m_Editor(e)
                , m_ChangeListener(listener)
            {
            }
            // Implemented below editor to use editor's interfaces
            virtual ~STransactionImplConnection()
            {
                m_Editor.RemoveChangeListener(m_ChangeListener);
            }
            QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Alloc, mRefCount);
        };

        void STransactionManagerImpl::addRef() { atomicIncrement(&mRefCount); }
        void STransactionManagerImpl::release()
        {
            atomicDecrement(&mRefCount);
            if (mRefCount <= 0) {
                TFoundationPtr fnd(m_EditorFoundation);
                NVDelete(fnd->getAllocator(), this);
            }
        }

        TSignalConnectionPtr
        STransactionManagerImpl::AddChangeListener(IEditorChangeListener &inListener)
        {
            m_ChangeListeners.push_back(&inListener);
            // If we have any listeners, then we can't be destroyed as their connections will still
            // need
            // to talk to us to release its stuff.
            if (m_ChangeListeners.size() == 1)
                addRef();
            return QT3DS_NEW(m_EditorFoundation->getAllocator(),
                          STransactionImplConnection)(m_EditorFoundation, *this, inListener);
        }

        void STransactionManagerImpl::RemoveChangeListener(IEditorChangeListener &inListener)
        {
            TChangeListenerList::iterator iter =
                eastl::find(m_ChangeListeners.begin(), m_ChangeListeners.end(), &inListener);
            if (iter != m_ChangeListeners.end())
                m_ChangeListeners.erase(iter);
            if (m_ChangeListeners.size() == 0)
                release();
        }
        TTransactionPtr STransactionManagerImpl::BeginTransaction(const TEditorStr &inName)
        {
            if (!m_Transaction) {
                QT3DS_ASSERT(m_OpenCount == 0);
                m_Transaction = QT3DS_NEW(m_EditorFoundation->getAllocator(),
                                       STransaction)(m_EditorFoundation, *this, inName);
            }
            ++m_OpenCount;
            return m_Transaction.mPtr;
        }

        TTransactionPtr STransactionManagerImpl::GetOpenTransaction() { return m_Transaction.mPtr; }
        STransaction *STransactionManagerImpl::GetOpenTransactionImpl()
        {
            return m_Transaction.mPtr;
        }

        void STransactionManagerImpl::RollbackTransaction()
        {
            --m_OpenCount;
            if (m_OpenCount <= 0) {
                if (m_Transaction) {
                    m_Transaction->Undo();
                    m_Transaction = NULL;
                }
            }
            QT3DS_ASSERT(m_OpenCount == 0);
            m_OpenCount = 0;
        }

        void STransactionManagerImpl::EndTransaction()
        {
            TTransactionPtr retval = m_Transaction.mPtr;
            --m_OpenCount;
            if (m_OpenCount <= 0)
                m_Transaction = NULL;
            QT3DS_ASSERT(m_OpenCount >= 0);
            m_OpenCount = 0;
        }

        void STransactionManagerImpl::OnObjectDeleted(TObjPtr inObj)
        {
            if (m_Transaction)
                m_Transaction->m_RemovedPairs.push_back(eastl::make_pair(true, inObj));

            if (m_ObjListener)
                m_ObjListener->OnObjectDeleted(inObj);
        }
        void STransactionManagerImpl::OnObjectCreated(TObjPtr inObj)
        {
            if (m_Transaction)
                m_Transaction->m_RemovedPairs.push_back(eastl::make_pair(false, inObj));

            if (m_ObjListener)
                m_ObjListener->OnObjectCreated(inObj);
        }
    }
}
}