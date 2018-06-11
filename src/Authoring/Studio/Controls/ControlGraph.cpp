/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#include "Qt3DSCommonPrecompile.h"
#include "ControlGraph.h"
#include "GraphImpl.h"
#include "foundation/Qt3DSAssert.h"
#include "Control.h"
#include "ControlData.h"

using namespace std;
using namespace Q3DStudio;
using namespace Q3DStudio::Graph;
using namespace Q3DStudio::Control;
using Q3DStudio::CString;

namespace {
typedef SGraphImpl<CControl *, std::shared_ptr<CControlData>> TGraphType;

TGraphType g_ControlGraph;
}

namespace Q3DStudio {
namespace Control {
    namespace ControlGraph {

        void AddNode(std::shared_ptr<CControlData> inData)
        {
            g_ControlGraph.AddRoot(inData->GetControl());
            g_ControlGraph.SetData(inData->GetControl(), inData);
        }

        void RemoveNode(CControl &inData) { g_ControlGraph.RemoveChild(&inData, true); }

        void AddChild(CControl &inParent, CControl &inChild, CControl *inNextSibling)
        {
            std::shared_ptr<CControlData> oldParent = GetParent(inChild);
            TGraphType::TNodePtr theParent(g_ControlGraph.GetImpl(&inParent));

            // It actually happens that sometimes inChild gets added with a sibling
            // but the sibling hasn't actually been added yet.
            if (inNextSibling != nullptr) {
                TGraphType::TNodePtr theSibling(g_ControlGraph.GetImpl(inNextSibling));
                if (theSibling->m_Parent == theParent)
                    g_ControlGraph.MoveBefore(&inChild, inNextSibling);
                else
                    g_ControlGraph.AddChild(&inParent, &inChild, SGraphPosition::SEnd());
            } else
                g_ControlGraph.AddChild(&inParent, &inChild, SGraphPosition::SEnd());

            if (!oldParent || oldParent->GetControl() != &inParent) {
                inChild.OnParentChanged(&inParent);
                if (oldParent)
                    oldParent->ChildrenChanged();
            }
            inChild.Invalidate();
            inParent.ChildrenChanged();
        }
        // inParent is supplied for error checking purposes.
        void RemoveChild(CControl &inParent, CControl &inChild)
        {
            inChild.OnParentChanged(nullptr);
            g_ControlGraph.RemoveChild(&inParent, &inChild, false);
            inChild.Invalidate();
            inParent.ChildrenChanged();
        }
        void RemoveAllChildren(CControl &inParent)
        {
            TGraphType::TNodePtr theNode(g_ControlGraph.GetImpl(&inParent));
            if (theNode == nullptr)
                return;
            while (theNode->m_Children.empty() == false) {
                theNode->m_Children.back()->m_Data->OnParentChanged(nullptr);
                theNode->m_Children.back()->m_Data->NotifyParentNeedsLayout();
                g_ControlGraph.RemoveChild(&inParent, theNode->m_Children.back()->m_GraphableID,
                                           false);
            }
            inParent.ChildrenChanged();
        }
        void MoveTo(CControl &inParent, CControl &inChild, const Graph::SGraphPosition &inPosition)
        {
            std::shared_ptr<CControlData> theOldParent(GetParent(inChild));
            g_ControlGraph.MoveTo(&inParent, &inChild, inPosition);
            if (!theOldParent || theOldParent->GetControl() != &inParent)
                inChild.OnParentChanged(&inParent);
            inChild.NotifyParentNeedsLayout();
            inParent.MarkChildrenNeedLayout();
            if (theOldParent) {
                theOldParent->ChildrenChanged();
            }
            inParent.ChildrenChanged();
            inChild.Invalidate();
        }
        long GetNumChildren(CControl &inControl)
        {
            return g_ControlGraph.GetChildCount(&inControl);
        }

        long GetChildIndex(CControl &inParent, CControl &inChild)
        {
            QT3DS_ASSERT(GetParent(inChild)->GetControl() == &inParent);

            SGraphPosition thePos = g_ControlGraph.GetNodePosition(&inChild);
            return thePos.GetIndex();
        }

        std::shared_ptr<CControlData> GetChild(CControl &inParent, long inIndex)
        {
            TGraphType::TNodePtr theNode(g_ControlGraph.GetImpl(&inParent));
            if (theNode && inIndex < (long)theNode->m_Children.size() && inIndex > -1)
                return theNode->m_Children[inIndex]->m_Data;
            return std::shared_ptr<CControlData>();
        }

        std::shared_ptr<CControlData> GetParent(CControl &inControl)
        {
            TGraphType::TNodePtr theNode(g_ControlGraph.GetImpl(&inControl));
            if (theNode && theNode->m_Parent)
                return theNode->m_Parent->m_Data;
            return std::shared_ptr<CControlData>();
        }

        // Dummy struct to hide graph implementation
        struct SDummyGraphNode : public TGraphType::TNodeType
        {
            SDummyGraphNode(CControl *const &inIdentifier,
                            const std::shared_ptr<CControlData> &inNodeData =
                                std::shared_ptr<CControlData>())
                : TGraphType::TNodeType(inIdentifier, inNodeData)
            {
            }
        };

        std::shared_ptr<CControlData> SIteratorBase::GetCurrent()
        {
            if (m_Children && m_Index < (long)m_Children->size())
                return (*m_Children)[m_Index]->m_Data;

            QT3DS_ASSERT(false);
            return std::shared_ptr<CControlData>();
        }

        SReverseIterator GetRChildren(CControl &inControl)
        {
            TGraphType::TNodePtr theNode(g_ControlGraph.GetImpl(&inControl));
            if (theNode)
                return SReverseIterator(
                    theNode->m_Data,
                    reinterpret_cast<vector<SDummyGraphNode *> &>(theNode->m_Children));
            return SReverseIterator();
        }

        SIterator GetChildren(CControl &inControl)
        {
            TGraphType::TNodePtr theNode(g_ControlGraph.GetImpl(&inControl));
            if (theNode)
                return SIterator(theNode->m_Data, reinterpret_cast<vector<SDummyGraphNode *> &>(
                                                      theNode->m_Children));
            return SIterator();
        }
    }
}
}
