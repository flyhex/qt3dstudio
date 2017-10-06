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
#pragma once
#ifndef GRAPHIMPLH
#define GRAPHIMPLH
#include <vector>
#include <unordered_map>
#include "GraphPosition.h"

namespace Q3DStudio {
namespace Graph {

    using namespace std;

    template <typename TKeyType, typename TValueType>
    struct SGraphNode
    {
        typedef SGraphNode<TKeyType, TValueType> TThisType;
        typedef vector<TThisType *> TNodeList;

        SGraphNode(const TKeyType &inIdentifier, const TValueType &inNodeData = TValueType())
            : m_GraphableID(inIdentifier)
            , m_Data(inNodeData)
            , m_Parent(NULL)
        {
        }

        SGraphNode(const SGraphNode&) = delete;
        SGraphNode& operator=(const SGraphNode&) = delete;

        TNodeList &GetParentList(TNodeList &inRoots)
        {
            return m_Parent != NULL ? m_Parent->m_Children : inRoots;
        }
        const TNodeList &GetParentList(const TNodeList &inRoots) const
        {
            return m_Parent != NULL ? m_Parent->m_Children : inRoots;
        }
        TKeyType GetParentID() const { return m_Parent != NULL ? m_Parent->m_GraphableID : 0; }
        TKeyType m_GraphableID; //< Opaque identifier
        TValueType m_Data;
        TThisType *m_Parent; //< Parent graphable
        TNodeList m_Children; //< Children graphables
    };

    // You are free to define different traits for different key types.
    template <typename TKeyType>
    struct SGraphKeyTraits
    {
        bool IsValid(const TKeyType &inKey) { return inKey != 0; }
        TKeyType Invalid() { return 0; }
    };

    template <typename TKeyType, typename TValueType>
    struct SGraphImpl
    {
        Q_DISABLE_COPY(SGraphImpl)
        SGraphImpl() = default;

    public:
        typedef SGraphNode<TKeyType, TValueType> TNodeType;
        typedef TNodeType *TNodePtr;
        typedef typename TNodeType::TNodeList TNodeList;
        typedef std::unordered_map<TKeyType, TNodePtr> TLookupMap;

        TNodeList m_Roots; //< Graph roots
        TLookupMap m_LookupMap; //< Mapping between TKeyType and TNodeType

        ~SGraphImpl() { ReleaseNodes(); }

        // Graph node lifetime and manipulation helpers
        void ReleaseNodes()
        {
            m_Roots.clear();
            for (typename TLookupMap::iterator iter = m_LookupMap.begin(), end = m_LookupMap.end();
                 iter != end; ++iter) {
                delete iter->second;
            }
            m_LookupMap.clear();
        }

        TNodePtr GetImpl(const TKeyType &inGraphable) const
        {
            if (SGraphKeyTraits<TKeyType>().IsValid(inGraphable)) {
                typename TLookupMap::const_iterator theResult = m_LookupMap.find(inGraphable);
                if (theResult != m_LookupMap.end())
                    return theResult->second;
            }
            return NULL;
        }

        pair<TNodePtr, bool> CreateImpl(const TKeyType &inGraphable,
                                        const TValueType &inValue = TValueType())
        {
            if (SGraphKeyTraits<TKeyType>().IsValid(inGraphable)) {
                TNodePtr theGraphableImpl = NULL;
                std::pair<typename TLookupMap::iterator, bool> theResult =
                    m_LookupMap.insert(std::make_pair(inGraphable, theGraphableImpl));

                if (theResult.second) {
                    theGraphableImpl = new TNodeType(inGraphable, inValue);
                    theResult.first->second = theGraphableImpl;
                }
                return make_pair(theResult.first->second, theResult.second);
            }
            assert(0);
            return make_pair(TNodePtr(NULL), false);
        }

        void SetData(const TKeyType &inGraphable, const TValueType &inData)
        {
            pair<TNodePtr, bool> theNode = CreateImpl(inGraphable, inData);
            if (theNode.second == false)
                theNode.first->m_Data = inData;
        }

        void DeleteImpl(TNodePtr item)
        {
            if (item) {
                m_LookupMap.erase(item->m_GraphableID);
                delete item;
            }
        }

        typename TNodeList::const_iterator FindNodeIter(const TNodePtr node,
                                                        const TNodePtr parent) const
        {
            if (node && parent) {
                typename TNodeList::const_iterator find =
                    std::find(parent->m_Children.begin(), parent->m_Children.end(), node);
                return find;
            }
            if (parent)
                return parent->m_Children.end();
            assert(0);
            return TNodeList::const_iterator();
        }

        long FindNode(const TNodePtr node, const TNodeList &inList) const
        {
            typename TNodeList::const_iterator find = std::find(inList.begin(), inList.end(), node);
            return (long)(find - inList.begin());
        }

        long FindNode(const TNodePtr node, const TNodePtr parent) const
        {
            if (node && parent)
                return (long)(FindNodeIter(node, parent) - parent->m_Children.begin());
            return 0;
        }

        long ResolveGraphPosition(TNodePtr theParent, SGraphPosition inPosition) const
        {
            const TNodeList &childList(theParent ? theParent->m_Children : m_Roots);
            return min(inPosition.GetIndex(), (long)childList.size());
        }

        long ResolveGraphPosition(TKeyType theParent, SGraphPosition inPosition) const
        {
            return ResolveGraphPosition(GetImpl(theParent), inPosition);
        }

        // Add child to either the indicated parent or the graph's root
        // list if no parent is indicated.
        long AddChild(TKeyType inParent, TKeyType inChild, SGraphPosition inIndex)
        {
            TNodePtr theChild(CreateImpl(inChild).first);
            assert(theChild->m_Parent == NULL);
            if (inParent) {
                TNodePtr theParent(GetImpl(inParent));
                assert(theParent);
                if (theParent == NULL)
                    return 0;
                theChild->m_Parent = theParent;
            }
            long theIndex = ResolveGraphPosition(theChild->m_Parent, inIndex);

            TNodeList &childList = theChild->GetParentList(m_Roots);

            if (theIndex <= (long)childList.size()) {
                childList.insert(childList.begin() + theIndex, theChild);
                return theIndex;
            } else {
                childList.push_back(theChild);
                return (long)childList.size() - 1;
            }
            return 0;
        }

        void AddRoot(const TKeyType inRoot) { AddChild(0, inRoot, SGraphPosition::SEnd()); }

        // Remove this child from either the parent list or from the roots list
        // if the parent is unspecified.
        // Furthermore, delete the child if specified.
        long RemoveChild(TKeyType inParent, TKeyType inChild, bool deleteChild)
        {
            TNodePtr theChild(GetImpl(inChild));

            if (theChild == NULL || theChild->GetParentID() != inParent)
                return 0;
            if (deleteChild) {
                while (theChild->m_Children.size())
                    RemoveChild(inChild, theChild->m_Children.back()->m_GraphableID, deleteChild);

                assert(theChild->m_Children.size() == 0);
            }

            TNodeList &childList = theChild->GetParentList(m_Roots);
            assert(theChild->m_Parent == GetImpl(inParent));
            theChild->m_Parent = NULL;

            long findIdx = FindNode(theChild, childList);
            if (findIdx < (long)childList.size())
                childList.erase(childList.begin() + findIdx);
            if (deleteChild)
                DeleteImpl(theChild);

            return findIdx;
        }

        long RemoveChild(TKeyType inChild, bool deleteChild)
        {
            TNodePtr theChild(GetImpl(inChild));
            if (theChild)
                return RemoveChild(theChild->GetParentID(), theChild->m_GraphableID, deleteChild);
            return 0;
        }

        void Move(const TKeyType inNode, long oldIdx, long newIdx)
        {
            TNodePtr node(GetImpl(inNode));
            if (node) {
                TNodeList &childList(node->GetParentList(m_Roots));
                assert(oldIdx < (long)childList.size());
                assert(newIdx < (long)childList.size());
                assert(childList.at(oldIdx) == node);
                childList.erase(childList.begin() + oldIdx);
                childList.insert(childList.begin() + newIdx, node);
                assert(childList.at(newIdx) == node);
            }
        }

        void ReParent(const TKeyType inNode, const TKeyType inNewParent)
        {
            RemoveChild(inNode, false);
            AddChild(inNewParent, inNode, SGraphPosition::SEnd());
        }

        void MoveChild(const TKeyType inChild, const SGraphPosition & /*inOldIdx*/,
                       const SGraphPosition &inNewIdx)
        {
            Move(inChild, inNewIdx);
        }

        void Move(const TKeyType inNode, SGraphPosition newIdx)
        {
            TNodePtr node(GetImpl(inNode));
            if (node) {
                SGraphPosition currentPos = GetNodePosition(inNode);
                long requestIdx = ResolveGraphPosition(node->GetParentID(), newIdx);
                Move(inNode, currentPos.GetIndex(), requestIdx);
            }
        }

        void Move(const TKeyType inNode, const TKeyType inOther, bool inAfter)
        {
            if (inNode == inOther)
                return;
            TNodePtr node(GetImpl(inNode));
            TNodePtr other(GetImpl(inOther));
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
                    TKeyType id = other->m_Parent ? other->m_Parent->m_GraphableID : 0;
                    AddChild(id, inNode, newPos);
                } else {
                    Move(inNode, newPos);
                }
                return;
            }
            assert(0);
        }

        void MoveBefore(const TKeyType inNode, const TKeyType inOther)
        {
            Move(inNode, inOther, false);
        }
        void MoveAfter(const TKeyType inNode, const TKeyType inOther)
        {
            Move(inNode, inOther, true);
        }

        void Insert(const TKeyType inNode, const TKeyType inOther, bool after)
        {
            if (GetImpl(inNode) == NULL)
                AddRoot(inNode);
            Move(inNode, inOther, after);
        }

        void InsertBefore(const TKeyType inNode, const TKeyType inOther)
        {
            Insert(inNode, inOther, false);
        }

        void InsertAfter(const TKeyType inNode, const TKeyType inOther)
        {
            Insert(inNode, inOther, true);
        }

        void MoveTo(const TKeyType inNode, const TKeyType inNewParent,
                    const SGraphPosition &inPosition)
        {
            if (GetParent(inNode) != inNewParent
                || !(GetNodePosition(inNode) == GetChildCount(inNewParent) - 1)) {
                RemoveChild(inNode, false);
                AddChild(inNewParent, inNode, inPosition);
            }
        }

        void InsertTo(const TKeyType inNode, const TKeyType inNewParent,
                      const SGraphPosition &inPosition)
        {
            AddChild(inNewParent, inNode, inPosition);
        }

        TKeyType GetParent(const TKeyType inChild) const
        {
            TNodePtr node = GetImpl(inChild);
            if (node && node->m_Parent)
                return node->m_Parent->m_GraphableID;
            return 0;
        }

        long GetChildCount(const TKeyType inParent) const
        {
            TNodePtr node = GetImpl(inParent);
            if (node)
                return (long)node->m_Children.size();
            return 0;
        }

        TKeyType GetChild(const TKeyType inParent, long inIndex) const
        {
            TNodePtr node = GetImpl(inParent);
            if (node && inIndex <= (long)node->m_Children.size())
                return node->m_Children[inIndex]->m_GraphableID;
            return 0;
        }

        TKeyType GetRoot(long inIndex) const
        {
            if (inIndex < (long)m_Roots.size())
                return m_Roots[inIndex]->m_GraphableID;
            return 0;
        }

        size_t GetRootsCount() const { return m_Roots.size(); }

        TKeyType GetSibling(const TKeyType inNode, bool inAfter) const
        {
            TNodePtr theNode = GetImpl(inNode);
            if (theNode) {
                const TNodeList &childList(theNode->GetParentList(m_Roots));
                typename TNodeList::const_iterator theChildPosition =
                    std::find(childList.begin(), childList.end(), theNode);
                if (inAfter) {
                    if (theChildPosition != childList.end())
                        ++theChildPosition;
                    if (theChildPosition != childList.end())
                        return (*theChildPosition)->m_GraphableID;
                } else {
                    if (theChildPosition != childList.begin()) {
                        --theChildPosition;
                        return (*theChildPosition)->m_GraphableID;
                    }
                }
            }
            return SGraphKeyTraits<TKeyType>().Invalid();
        }

        bool IsExist(const TKeyType inGraphable) const
        {
            typename TLookupMap::const_iterator theFind = m_LookupMap.find(inGraphable);
            return theFind != m_LookupMap.end();
        }

        SGraphPosition GetNodePosition(const TKeyType inNode) const
        {
            TNodePtr theNode(GetImpl(inNode));
            if (theNode) {
                const TNodeList &childList(theNode->GetParentList(m_Roots));
                typename TNodeList::const_iterator thePos =
                    std::find(childList.begin(), childList.end(), theNode);
                return (long)(thePos - childList.begin());
            }
            return SGraphPosition::SInvalid();
        }
    };
}
}

#endif
