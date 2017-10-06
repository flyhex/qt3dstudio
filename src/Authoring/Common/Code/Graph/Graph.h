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

//==============================================================================
//	Include
//==============================================================================
#pragma once
#include <vector>
#include <map>
#include "SIterator.h"

namespace UICDM {
class ITransactionConsumer;
class ISignalConnection;
}

namespace Q3DStudio {

typedef int TIdentifier;
typedef std::vector<TIdentifier> TGraphableList;

typedef enum _ECriteriaType {
    ECriteriaType_Sorter, //< Sorts using a compare function
    ECriteriaType_Filter, //< Filters using a boolean function
    ECriteriaType_Unknown
} ECriteriaType;

typedef std::function<bool(const TIdentifier inFirst, const TIdentifier inSecond)> TSorter;
typedef std::function<bool(const TIdentifier inGraphable)> TFilter;
typedef bool (*TSorterFptr)(const TIdentifier inFirst, const TIdentifier inSecond);
typedef bool (*TFilterFptr)(const TIdentifier inGraphable);

typedef struct _SGraphableCriteria
{
    ECriteriaType m_Type; //< Criteria type
    TSorter m_SortFunc; //< Sort function
    TFilter m_FilterFunc; //< Filter function

    _SGraphableCriteria(const TSorter &inFunction)
        : m_Type(ECriteriaType_Sorter)
        , m_SortFunc(inFunction)
        , m_FilterFunc(NULL)
    {
    }

    _SGraphableCriteria(const TFilter &inFunction)
        : m_Type(ECriteriaType_Filter)
        , m_SortFunc(NULL)
        , m_FilterFunc(inFunction)
    {
    }

    _SGraphableCriteria(const TSorterFptr inFunction)
        : m_Type(ECriteriaType_Sorter)
        , m_SortFunc(inFunction)
        , m_FilterFunc(NULL)
    {
    }

    _SGraphableCriteria(const TFilterFptr inFunction)
        : m_Type(ECriteriaType_Filter)
        , m_SortFunc(NULL)
        , m_FilterFunc(inFunction)
    {
    }

} SGraphableCriteria;

typedef std::vector<SGraphableCriteria> TCriteriaList;

//==============================================================================
/**
 *	Generic iterator class that contains a list of criterias (sorters or filters).
 *	Criterias are applied in the order they are added.
 *	Results can then be obtained from the class.
 */
class CGraphIterator : public CSIterator<TIdentifier>
{
public:
    CGraphIterator();
    CGraphIterator(const SGraphableCriteria &inCriteria);
    CGraphIterator(const TCriteriaList &inCriterias);

    virtual ~CGraphIterator() {}

    // Hide copy constructor
    // Copying an iterator could be really heavy, consider if there are alternatives to copying if
    // possible
protected:
    CGraphIterator &operator=(const CGraphIterator &) { return *this; };
    CGraphIterator(const CGraphIterator &){};

    // CSIterator overrides
public:
    bool IsDone() override;
    void operator++() override;
    void operator+=(const long inNumToInc) override;
    TIdentifier GetCurrent() override;

public:
    size_t GetCount() const;
    TIdentifier GetResult(size_t inIndex) const;
    void Reset();
    void ClearResults();
    void AddResult(TIdentifier result) { m_Results.push_back(result); }

    // Criteria management
public:
    void operator+=(const SGraphableCriteria &inCriteria);
    void operator+=(const TCriteriaList &inCriterias);
    void ClearCriterias();
    void ApplyCriterias();

protected:
    friend class CGraph;
    TGraphableList m_Results; //< Results list
    TCriteriaList m_Criterias; //< Criterias list

protected:
    TGraphableList::iterator m_Current; //< Current std::iterator
    TGraphableList::const_iterator m_End; //< End std::iterator
};

//==============================================================================
/**
 *	Opaque implementation for storing node position for move/insert operations
 */
class COpaquePosition
{
public:
    static const COpaquePosition FIRST;
    static const COpaquePosition LAST;
    static const long s_Invalid = -2;

    COpaquePosition()
        : m_Index(s_Invalid)
    {
    }
    COpaquePosition(long inIndex)
        : m_Index(inIndex)
    {
    }
    bool operator==(const COpaquePosition &other) const { return m_Index == other.m_Index; }
    long GetIndex() const { return m_Index; }
    bool IsValid() const { return m_Index != s_Invalid; }

private:
    long m_Index;
};
//==============================================================================
/**
 *	A CGraph is a directed graph with a set of roots nodes.  Any node without a
 *	parent is assumed to be a root node.  Removing nodes from the graph
 *	recursively removes children and the graph recycles their internal
 *	internal representation.
 *	Children of nodes are ordered as are root nodes.
 */
class CGraph
{
public:
    virtual ~CGraph(){}
    virtual void SetConsumer(std::shared_ptr<UICDM::ITransactionConsumer> inConsumer) = 0;

    /**
     *	Add a new child to the graph.  If inParent == NULL then it is added to the list of roots.
     */
    virtual void AddChild(const TIdentifier inParent, const TIdentifier inChild,
                          COpaquePosition inIndex = COpaquePosition::LAST) = 0;

    /**
     *	If deleteNode is true, this recursively deletes all children also to avoid
     *	dangling pointers.  If deleteNode is false, then the child is in limbo, neither with a
     *parent
     *	Nor in the root list.
     */
    virtual void RemoveChild(const TIdentifier inChild, bool deleteNode) = 0;

    /**
     *	Postcondition of remove node is that IsExist = false, recursively delete any attached
     *children
     */
    void RemoveNode(const TIdentifier inChild) { RemoveChild(inChild, true); }
    /**
     *	Move a child from oldIdx (which must be its existing index) to a new index
     */
    virtual void MoveChild(const TIdentifier inChild, const COpaquePosition &inOldIdx,
                           const COpaquePosition &inNewIdx) = 0;
    /**
     * Add a new root to the graph.
     */
    void AddRoot(const TIdentifier inRoot) { AddChild(0, inRoot); }

    /**
     *	Graph mutators that can be described by the operations above.
     *
     */
    virtual void ReParent(const TIdentifier inNode, const TIdentifier inNewParent) = 0;
    /**
     *	Move requires the node to be in existance.  InBefore or inAfter do not need
     *	to share a parent with inNode.
     */
    virtual void MoveBefore(const TIdentifier inNode, const TIdentifier inBefore) = 0;
    virtual void MoveAfter(const TIdentifier inNode, const TIdentifier inAfter) = 0;
    /**
     *	Insert will create the node before placing it in the correct spot.
     *	InBefore or inAfter do not need to share a parent with inNode.
     */
    virtual void InsertBefore(const TIdentifier inNode, const TIdentifier inBefore) = 0;
    virtual void InsertAfter(const TIdentifier inNode, const TIdentifier inAfter) = 0;

    /**
     *	Move a node to a different parent.  This help you when you don't know the child sibling.
     */
    virtual void MoveTo(const TIdentifier inNode, const TIdentifier inNewParent,
                        const COpaquePosition &inPosition) = 0;
    virtual void InsertTo(const TIdentifier inNode, const TIdentifier inNewParent,
                          const COpaquePosition &inPosition) = 0;

    // Not an undoable operation!!
    virtual void Clear() = 0;

    // Simple graph queries
    virtual TIdentifier GetParent(const TIdentifier inChild) const = 0;
    virtual long GetChildCount(const TIdentifier inParent) const = 0;
    virtual TIdentifier GetChild(const TIdentifier inParent, long inIndex) const = 0;
    virtual TIdentifier GetRoot(long inIndex) const = 0;
    virtual size_t GetRootsCount() const = 0;

    /**
     *	Return the sibling of this node.  Node may be a root node.
     */
    virtual TIdentifier GetSibling(const TIdentifier inNode, bool inAfter) const = 0;

    virtual bool IsExist(const TIdentifier inGraphable) const = 0;
    virtual COpaquePosition GetNodePosition(const TIdentifier inNode) const = 0;

    //  Iterators
    virtual void GetRoots(CGraphIterator &outIterator) const = 0;
    virtual void GetReverseRoots(CGraphIterator &outIterator) const = 0;
    virtual void GetChildren(CGraphIterator &outIterator, const TIdentifier inParent) const = 0;
    virtual void GetReverseChildren(CGraphIterator &outIterator,
                                    const TIdentifier inParent) const = 0;
    virtual void GetDepthFirst(CGraphIterator &outIterator, const TIdentifier inRoot) const = 0;

    // Signals
    // Always in the form of parent,child,childIdx
    virtual std::shared_ptr<UICDM::ISignalConnection>
    ConnectChildAdded(std::function<void(TIdentifier, TIdentifier, long)> inCallback) = 0;
    virtual std::shared_ptr<UICDM::ISignalConnection>
    ConnectChildRemoved(std::function<void(TIdentifier, TIdentifier, long)> inCallback) = 0;
    // parent,child,oldpos,newpos
    virtual std::shared_ptr<UICDM::ISignalConnection>
    ConnectChildMoved(std::function<void(TIdentifier, TIdentifier, long, long)> inCallback) = 0;

    static std::shared_ptr<CGraph> CreateGraph(TIdentifier inRoot = 0);
};

typedef std::shared_ptr<CGraph> TGraphPtr;

} // namespace Q3DStudio
