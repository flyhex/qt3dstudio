/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#include "stdafx.h"
#include "Qt3DSDMPrefix.h"
#include "Qt3DSDMTransactions.h"
#include "Qt3DSDMSignals.h"
#include "Graph.h"
#include "GraphImpl.h"

using namespace Q3DStudio;
using namespace qt3dsdm;

//==============================================================================
//	Statics
//==============================================================================
const COpaquePosition COpaquePosition::FIRST = COpaquePosition(0);
const COpaquePosition COpaquePosition::LAST = COpaquePosition(-1);

namespace {

#define CONNECT(x) TSignalConnectionPtr(new qt3dsdm::QtSignalConnection(x))
#define CONNECT_SIGNAL(x) CONNECT(QObject::connect(this,x,inCallback))

typedef Q3DStudio::Graph::SGraphNode<int, int> SGraphableImpl;
typedef SGraphableImpl *TGraphableImplPtr;
typedef SGraphableImpl::TNodeList TGraphableImplList;

struct SGraphImpl;
typedef SGraphImpl *TGraphImplPtr;

// Transaction events are implemented by this small set of satellite classes.
// These classes are stored in the transaction consumer and are called upone
// to do or undo particular changes to the graph.

struct SAddRemoveChildAction
{
    TGraphImplPtr m_Graph;
    TIdentifier m_Parent;
    TIdentifier m_Child;
    long m_Index;
    bool m_DeleteOnRemove;

    SAddRemoveChildAction(TGraphImplPtr graph, TIdentifier parent, TIdentifier child, long idx,
                          bool deleteOnRemove)
        : m_Graph(graph)
        , m_Parent(
              parent) // Parent may be null.  Removing a child from null means removing from root.
        , m_Child(child)
        , m_Index(idx)
        , m_DeleteOnRemove(deleteOnRemove)
    {
    }
    void AddChild();
    void RemoveChild();
};

struct SInsertChildTransaction : public qt3dsdm::ITransaction, public SAddRemoveChildAction
{
    SInsertChildTransaction(const char *inFile, int inLine, const SAddRemoveChildAction &action)
        : qt3dsdm::ITransaction(inFile, inLine)
        , SAddRemoveChildAction(action)
    {
    }
    void Do() override { AddChild(); }
    void Undo() override { RemoveChild(); }
};

struct SRemoveChildTransaction : public qt3dsdm::ITransaction, public SAddRemoveChildAction
{
    SRemoveChildTransaction(const char *inFile, int inLine, const SAddRemoveChildAction &action)
        : qt3dsdm::ITransaction(inFile, inLine)
        , SAddRemoveChildAction(action)
    {
    }
    void Do() override { RemoveChild(); }
    void Undo() override { AddChild(); }
};

struct SReorderChildTransaction : public qt3dsdm::ITransaction
{
    TGraphImplPtr m_Graph;
    TIdentifier m_Parent;
    TIdentifier m_Child;
    long m_OldIndex;
    long m_NewIndex;

    SReorderChildTransaction(const char *inFile, int inLine, TGraphImplPtr graph,
                             TIdentifier parent, TIdentifier child, long oldIdx, long newIdx)
        : qt3dsdm::ITransaction(inFile, inLine)
        , m_Graph(graph)
        , m_Parent(parent)
        , m_Child(child)
        , m_OldIndex(oldIdx)
        , m_NewIndex(newIdx)
    {
    }
    void Do() override;
    void Undo() override;
};

static inline Graph::SGraphPosition ToGraph(const COpaquePosition &inPos)
{
    if (inPos == COpaquePosition::LAST)
        return Graph::SGraphPosition::SEnd();
    if (inPos == COpaquePosition::FIRST)
        return Graph::SGraphPosition::SBegin();
    return inPos.GetIndex();
}

static inline COpaquePosition FromGraph(Graph::SGraphPosition inPos)
{
    switch (inPos.GetType()) {
    case Graph::SGraphPosition::Begin:
        return COpaquePosition::FIRST;
    case Graph::SGraphPosition::End:
        return COpaquePosition::LAST;
    case Graph::SGraphPosition::Invalid:
        return COpaquePosition::s_Invalid;
    case Graph::SGraphPosition::Index:
        return inPos.GetIndex();
    }
    Q_ASSERT(0);
    return COpaquePosition::s_Invalid;
}

// End section of graph transaction helpers.

//==============================================================================
/**
 *	Graph implementation.
 *	Graph mutator implementation is broken up into 2 components.  There are external
 *	components that check arguments may send transactions and internal components that
 *	expect correct arguments and can be driven either by events or transactions.
 *	Methods start with "Do" are the internal interface used by the graph and by
 *	transactions to change graph state.
 */
class SGraphImpl : public QObject, public CGraph
{
    Q_OBJECT
public:
    typedef std::map<TIdentifier, TGraphableImplPtr> TLookupMap;
    SGraphImpl(TIdentifier inRoot)
    {
        if (inRoot != 0)
            CreateImpl(inRoot);
    }
    void SetConsumer(std::shared_ptr<qt3dsdm::ITransactionConsumer> inConsumer) override
    {
        m_Consumer = inConsumer;
    }

    virtual ~SGraphImpl() { ReleaseNodes(); }

    // Graph node lifetime and manipulation helpers
    void ReleaseNodes() { m_Graph.ReleaseNodes(); }

    TGraphableImplPtr GetImpl(const TIdentifier inGraphable) const
    {
        return m_Graph.GetImpl(inGraphable);
    }

    TGraphableImplPtr CreateImpl(const TIdentifier inGraphable)
    {
        std::pair<TGraphableImplPtr, bool> result = m_Graph.CreateImpl(inGraphable);
        Q_ASSERT(result.second);
        return result.first;
    }

    TGraphableImplPtr GetOrCreateImpl(const TIdentifier inGraphable)
    {
        return m_Graph.CreateImpl(inGraphable).first;
    }

    void DeleteImpl(TGraphableImplPtr item) { m_Graph.DeleteImpl(item); }

    TGraphableImplList::const_iterator FindNodeIter(const TGraphableImplPtr node,
                                                    const TGraphableImplPtr parent) const
    {
        if (node && parent) {
            TGraphableImplList::const_iterator find =
                std::find(parent->m_Children.begin(), parent->m_Children.end(), node);
            return find;
        }
        if (parent)
            return parent->m_Children.end();
        Q_ASSERT(0);
        return TGraphableImplList::const_iterator();
    }

    long FindNode(const TGraphableImplPtr node, const TGraphableImplList &inList)
    {
        TGraphableImplList::const_iterator find = std::find(inList.begin(), inList.end(), node);
        return (long)(find - inList.begin());
    }

    long FindNode(const TGraphableImplPtr node, const TGraphableImplPtr parent)
    {
        if (node && parent)
            return (long)(FindNodeIter(node, parent) - parent->m_Children.begin());
        return 0;
    }

    long ResolveOpaquePosition(TGraphableImplPtr theParent, COpaquePosition inPosition) const
    {
        return m_Graph.ResolveGraphPosition(theParent, ToGraph(inPosition));
    }

    long ResolveOpaquePosition(TIdentifier theParent, COpaquePosition inPosition) const
    {
        Graph::SGraphPosition thePos(ToGraph(inPosition));
        return m_Graph.ResolveGraphPosition(theParent, thePos);
    }

    // Add child to either the indicated parent or the graph's root
    // list if no parent is indicated.
    long DoAddChild(TIdentifier inParent, TIdentifier inChild, long inIndex)
    {
        return m_Graph.AddChild(inParent, inChild, inIndex);
    }

    // Remove this child from either the parent list or from the roots list
    // if the parent is unspecified.
    // Furthermore, delete the child if specified.
    long DoRemoveChild(TIdentifier inParent, TIdentifier inChild, bool deleteChild)
    {
        return m_Graph.RemoveChild(inParent, inChild, deleteChild);
    }

    // Recursively delete sub children of the graph node
    void ClearChildren(TGraphableImplPtr theChild)
    {
        while (theChild->m_Children.size())
            RemoveChild(theChild->m_Children[0]->m_GraphableID, true);
    }

    void DoMove(const TIdentifier inNode, long oldIdx, long newIdx)
    {
        m_Graph.Move(inNode, oldIdx, newIdx);
    }

    void MoveWithEvent(const TIdentifier inNode, long newIdx)
    {
        TGraphableImplPtr node(GetImpl(inNode));
        if (node) {
            TIdentifier theParent(node->GetParentID());
            long oldIdx = FindNode(node, node->GetParentList(m_Graph.m_Roots));
            if (oldIdx != newIdx) {
                DoMove(inNode, oldIdx, newIdx);
                if (m_Consumer != NULL) {
                    m_Consumer->OnTransaction(std::make_shared<SReorderChildTransaction>(
                        __FILE__, __LINE__, this, node->GetParentID(), inNode, oldIdx, newIdx));
                    m_Consumer->OnDoNotification(bind(&SGraphImpl::SignalChildMoved, this,
                                                      theParent, inNode, oldIdx, newIdx));
                    m_Consumer->OnUndoNotification(bind(&SGraphImpl::SignalChildMoved, this,
                                                        theParent, inNode, newIdx, oldIdx));
                }
            }
        }
    }

    // Begin external mutator interfaces

    void AddChild(const TIdentifier inParent, const TIdentifier inChild,
                          COpaquePosition inPosition) override
    {
        bool created = false;
        TGraphableImplPtr theChild(GetImpl(inChild));
        if (theChild == NULL) {
            theChild = CreateImpl(inChild);
            created = true;
        }
        Q_ASSERT(theChild->m_Parent == NULL);
        if (theChild) {
            long theIndex = ResolveOpaquePosition(inParent, inPosition);
            theIndex = DoAddChild(inParent, inChild, theIndex);
            if (m_Consumer) {
                m_Consumer->OnTransaction(std::static_pointer_cast<ITransaction>(
                    std::make_shared<SInsertChildTransaction>(
                        __FILE__, __LINE__,
                        SAddRemoveChildAction(this, inParent, inChild, theIndex, created))));

                m_Consumer->OnDoNotification(
                    bind(&SGraphImpl::SignalChildAdded, this, inParent, inChild, theIndex));
                m_Consumer->OnUndoNotification(
                    bind(&SGraphImpl::SignalChildRemoved, this, inParent, inChild, theIndex));
            }
        }
    }

    void RemoveChild(const TIdentifier inNode, bool deleteNode) override
    {
        TGraphableImplPtr theChild(GetImpl(inNode));
        if (theChild) {
            if (deleteNode)
                ClearChildren(theChild);
            TIdentifier theParent = theChild->GetParentID();
            long removeChildIdx = DoRemoveChild(theParent, inNode, deleteNode);
            if (m_Consumer != NULL) {
                m_Consumer->OnTransaction(std::static_pointer_cast<ITransaction>(
                    std::make_shared<SRemoveChildTransaction>(
                        __FILE__, __LINE__, SAddRemoveChildAction(this, theParent, inNode,
                                                                  removeChildIdx, deleteNode))));

                m_Consumer->OnDoNotification(
                    bind(&SGraphImpl::SignalChildRemoved, this, theParent, inNode, removeChildIdx));
                m_Consumer->OnUndoNotification(
                    bind(&SGraphImpl::SignalChildAdded, this, theParent, inNode, removeChildIdx));
            }
        }
    }

    void ReParent(const TIdentifier inNode, const TIdentifier inNewParent) override
    {
        RemoveChild(inNode, false);
        AddChild(inNewParent, inNode, COpaquePosition::FIRST);
    }

    void MoveChild(const TIdentifier inChild, const COpaquePosition & /*inOldIdx*/,
                           const COpaquePosition &inNewIdx) override
    {
        Move(inChild, inNewIdx);
    }

    void Move(const TIdentifier inNode, COpaquePosition newIdx)
    {
        TGraphableImplPtr node(GetImpl(inNode));
        if (node) {
            long requestIdx = ResolveOpaquePosition(node->GetParentID(), newIdx);
            MoveWithEvent(inNode, requestIdx);
        }
    }

    virtual void Move(const TIdentifier inNode, const TIdentifier inOther, bool inAfter)
    {
        if (inNode == inOther)
            return;
        TGraphableImplPtr node(GetOrCreateImpl(inNode));
        TGraphableImplPtr other(GetOrCreateImpl(inOther));
        if (node && other) {
            long newPos = GetNodePosition(inOther).GetIndex();
            long currentPos = GetNodePosition(inNode).GetIndex();

            // We know we will do an erase followed by an insert.
            if (node->GetParentID() == other->GetParentID()) {
                if (newPos > currentPos) {
                    if (inAfter == false)
                        --newPos;
                }

                if (newPos < currentPos) {
                    if (inAfter == true)
                        ++newPos;
                }
            } else {
                if (inAfter)
                    ++newPos;
            }

            if (node->m_Parent != other->m_Parent) {
                RemoveChild(inNode, false);
                TIdentifier id = other->m_Parent ? other->m_Parent->m_GraphableID : 0;
                AddChild(id, inNode, newPos);
            } else {
                MoveWithEvent(inNode, newPos);
            }
            return;
        }
        Q_ASSERT(0);
    }

    void MoveBefore(const TIdentifier inNode, const TIdentifier inOther) override
    {
        Move(inNode, inOther, false);
    }
    void MoveAfter(const TIdentifier inNode, const TIdentifier inOther) override
    {
        Move(inNode, inOther, true);
    }

    virtual void Insert(const TIdentifier inNode, const TIdentifier inOther, bool after)
    {
        if (GetImpl(inNode) == NULL)
            AddRoot(inNode);
        Move(inNode, inOther, after);
    }

    void InsertBefore(const TIdentifier inNode, const TIdentifier inOther) override
    {
        Insert(inNode, inOther, false);
    }

    void InsertAfter(const TIdentifier inNode, const TIdentifier inOther) override
    {
        Insert(inNode, inOther, true);
    }

    void MoveTo(const TIdentifier inNode, const TIdentifier inNewParent,
                        const COpaquePosition &inPosition) override
    {
        if (GetParent(inNode) != inNewParent
                || !(GetNodePosition(inNode) == GetChildCount(inNewParent) - 1)) {
            RemoveChild(inNode, false);
            AddChild(inNewParent, inNode, inPosition);
        }
    }

    void InsertTo(const TIdentifier inNode, const TIdentifier inNewParent,
                          const COpaquePosition &inPosition) override
    {
        AddChild(inNewParent, inNode, inPosition);
    }

    TIdentifier GetParent(const TIdentifier inChild) const override
    {
        return m_Graph.GetParent(inChild);
    }
    long GetChildCount(const TIdentifier inParent) const override
    {
        return m_Graph.GetChildCount(inParent);
    }

    TIdentifier GetChild(const TIdentifier inParent, long inIndex) const override
    {
        return m_Graph.GetChild(inParent, inIndex);
    }

    TIdentifier GetRoot(long inIndex) const override { return m_Graph.GetRoot(inIndex); }

    size_t GetRootsCount() const override { return m_Graph.GetRootsCount(); }

    TIdentifier GetSibling(const TIdentifier inNode, bool inAfter) const override
    {
        return m_Graph.GetSibling(inNode, inAfter);
    }

    bool IsExist(const TIdentifier inGraphable) const override
    {
        return m_Graph.IsExist(inGraphable);
    }

    COpaquePosition GetNodePosition(const TIdentifier inNode) const override
    {
        return FromGraph(m_Graph.GetNodePosition(inNode));
    }

    void Clear() override { ReleaseNodes(); }

    template <typename TIteratorType>
    void GetItems(CGraphIterator &outIterator, TIteratorType begin, TIteratorType end) const
    {
        outIterator.ClearResults();
        for (TIteratorType theIter = begin; theIter != end; ++theIter)
            outIterator.AddResult((*theIter)->m_GraphableID);
        outIterator.ApplyCriterias();
    }

    //  Iterators
    void GetRoots(CGraphIterator &outIterator) const override
    {
        GetItems(outIterator, m_Graph.m_Roots.begin(), m_Graph.m_Roots.end());
    }

    void GetReverseRoots(CGraphIterator &outIterator) const override
    {
        GetItems(outIterator, m_Graph.m_Roots.rbegin(), m_Graph.m_Roots.rend());
    }

    void GetChildren(CGraphIterator &outIterator, const TIdentifier inParent) const override
    {
        const TGraphableImplPtr parent = GetImpl(inParent);
        const TGraphableImplList &childList(parent ? parent->m_Children : m_Graph.m_Roots);
        GetItems(outIterator, childList.begin(), childList.end());
    }
    void GetReverseChildren(CGraphIterator &outIterator, const TIdentifier inParent) const override
    {
        const TGraphableImplPtr parent = GetImpl(inParent);
        const TGraphableImplList &childList(parent ? parent->m_Children : m_Graph.m_Roots);
        GetItems(outIterator, childList.rbegin(), childList.rend());
    }
    void GetDepthFirst(CGraphIterator &outIterator, const TGraphableImplPtr inRoot) const
    {
        if (inRoot == NULL) {
            Q_ASSERT(0);
            return;
        }
        outIterator.AddResult(inRoot->m_GraphableID);
        for (size_t idx = 0, size = inRoot->m_Children.size(); idx < size; ++idx)
            GetDepthFirst(outIterator, inRoot->m_Children[idx]);
    }

    void GetDepthFirst(CGraphIterator &outIterator, const TIdentifier inRoot) const override
    {
        if (inRoot != 0) {
            TGraphableImplPtr root = GetImpl(inRoot);
            if (root)
                GetDepthFirst(outIterator, root);
            else {
                Q_ASSERT(0);
            }
        } else {
            for (size_t idx = 0, size = m_Graph.m_Roots.size(); idx < size; ++idx)
                GetDepthFirst(outIterator, m_Graph.m_Roots[idx]);
        }
        outIterator.ApplyCriterias();
    }

    void SignalChildAdded(TIdentifier inParent, TIdentifier inChild, long inPos)
    {
        Q_EMIT childAddedSignal(inParent, inChild, inPos);
    }
    void SignalChildRemoved(TIdentifier inParent, TIdentifier inChild, long inPos)
    {
        Q_EMIT childRemoveSignal(inParent, inChild, inPos);
    }
    void SignalChildMoved(TIdentifier inParent, TIdentifier inChild, long inOldIdx, long inNewIdx)
    {
        Q_EMIT childMoved(inParent, inChild, inOldIdx, inNewIdx);
    }
    // Signals
    // Always in the form of parent,child.
    virtual std::shared_ptr<qt3dsdm::ISignalConnection>
    ConnectChildAdded(std::function<void(TIdentifier, TIdentifier, long)> inCallback) override
    {
        return CONNECT_SIGNAL(&SGraphImpl::childAddedSignal);
    }
    virtual std::shared_ptr<qt3dsdm::ISignalConnection>
    ConnectChildRemoved(std::function<void(TIdentifier, TIdentifier, long)> inCallback) override
    {
        return CONNECT_SIGNAL(&SGraphImpl::childRemoveSignal);
    }
    virtual std::shared_ptr<qt3dsdm::ISignalConnection>
    ConnectChildMoved(std::function<void(TIdentifier, TIdentifier, long, long)> inCallback) override
    {
        return CONNECT_SIGNAL(&SGraphImpl::childMoved);
    }

    Graph::SGraphImpl<int, int> m_Graph;
    std::shared_ptr<qt3dsdm::ITransactionConsumer> m_Consumer;
Q_SIGNALS:
    void childAddedSignal(TIdentifier, TIdentifier, long);
    void childRemoveSignal(TIdentifier, TIdentifier, long);
    void childMoved(TIdentifier, TIdentifier, long, long);
};

// Implementation of the transaction helpers.
void SAddRemoveChildAction::AddChild()
{
    m_Graph->DoAddChild(m_Parent, m_Child, m_Index);
}
void SAddRemoveChildAction::RemoveChild()
{
    m_Graph->DoRemoveChild(m_Parent, m_Child, m_DeleteOnRemove);
}
void SReorderChildTransaction::Do()
{
    m_Graph->DoMove(m_Child, m_OldIndex, m_NewIndex);
}
void SReorderChildTransaction::Undo()
{
    m_Graph->DoMove(m_Child, m_NewIndex, m_OldIndex);
}

} // end anonymous namespace

//==============================================================================
/**
 *	CTOR
 */
CGraphIterator::CGraphIterator()
{
    Reset();
}

//==============================================================================
/**
 *	CTOR
 *	Accepts a single criteria
 */
CGraphIterator::CGraphIterator(const SGraphableCriteria &inCriteria)
{
    m_Criterias.push_back(inCriteria);
}

//==============================================================================
/**
 *	CTOR
 *	Accepts a list of criterias
 */
CGraphIterator::CGraphIterator(const TCriteriaList &inCriterias)
    : m_Criterias(inCriterias)
{
}

//==============================================================================
/**
 *	SIterator override
 *	Returns true when iterator is at it's end
 */
bool CGraphIterator::IsDone()
{
    return m_Current == m_End;
}

//==============================================================================
/**
 *	SIterator override
 *	Advance iterator
 */
void CGraphIterator::operator++()
{
    ++m_Current;
}

//==============================================================================
/**
 *	SIterator override
 *	Advance iterator by a certain amount
 */
void CGraphIterator::operator+=(const long inNumToInc)
{
    m_Current += inNumToInc;
}

//==============================================================================
/**
 *	SIterator override
 *	Returns current item
 */
TIdentifier CGraphIterator::GetCurrent()
{
    return *m_Current;
}

//==============================================================================
/**
 *	Intializes iterator back to start state
 */
void CGraphIterator::Reset()
{
    m_Current = m_Results.begin();
    m_End = m_Results.end();
}

//==============================================================================
/**
 *	Clear the results
 */
void CGraphIterator::ClearResults()
{
    m_Results.clear();
    Reset();
}

//==============================================================================
/**
 * Get the size of results
 */
size_t CGraphIterator::GetCount() const
{
    return m_Results.size();
}

//==============================================================================
/**
 * Get the result at certain index
 */
TIdentifier CGraphIterator::GetResult(size_t inIndex) const
{
    return m_Results[inIndex];
}

//==============================================================================
/**
 *	Adds a set of criterias to the existing set
 */
void CGraphIterator::operator+=(const TCriteriaList &inCriterias)
{
    m_Criterias.insert(m_Criterias.end(), inCriterias.begin(), inCriterias.end());
}

//==============================================================================
/**
 *	Adds a new criteria to the existing set
 */
void CGraphIterator::operator+=(const SGraphableCriteria &inCriteria)
{
    m_Criterias.push_back(inCriteria);
}

//==============================================================================
/**
 * Clears the current list of criterias
 */
void CGraphIterator::ClearCriterias()
{
    m_Criterias.clear();
}

//==============================================================================
/**
 *	Evaluates an criterias for an iterator
 */
void CGraphIterator::ApplyCriterias()
{
    TCriteriaList::const_iterator theIter = m_Criterias.begin();
    TCriteriaList::const_iterator theEnd = m_Criterias.end();

    for (; theIter != theEnd; ++theIter) {
        const SGraphableCriteria &theCriteria = *theIter;
        switch (theCriteria.m_Type) {
        case ECriteriaType_Sorter:
            std::stable_sort(m_Results.begin(), m_Results.end(), theCriteria.m_SortFunc);
            break;

        case ECriteriaType_Filter:
            TGraphableList::iterator theNewEnd;
            theNewEnd =
                std::remove_if(m_Results.begin(), m_Results.end(),
                               theCriteria.m_FilterFunc); // exclude those that satisfy the filter
            m_Results.erase(theNewEnd, m_Results.end()); // clear away the junk
            break;
        }
    }

    // Get this iterator ready for use
    Reset();
}

std::shared_ptr<CGraph> CGraph::CreateGraph(TIdentifier inRoot)
{
    return std::static_pointer_cast<CGraph>(std::make_shared<SGraphImpl>(inRoot));
}

#include "Graph.moc"

