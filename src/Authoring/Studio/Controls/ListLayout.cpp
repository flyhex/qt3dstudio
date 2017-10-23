/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#include "ListLayout.h"
#include "MasterP.h"
#include "ControlData.h"

using namespace Q3DStudio;
using namespace Q3DStudio::Control;

CListLayout::CListLayout(bool inAlignChildrenLength /*= false */)
    : m_IsResizing(false)
    , m_AlignChildrenLength(inAlignChildrenLength)
{
}

CListLayout::~CListLayout()
{
}

//=============================================================================
/**
 * Add a child to this control.
 * Overrides to make the layout get recalculated as well.
 * @param inChild the child to be added.
 * @param inInsertBefore the location to be added.
 * @see CControl::AddChild.
 */
void CListLayout::AddChild(CControl *inChild, CControl *inInsertBefore /*= nullptr*/)
{
    CControl::AddChild(inChild, inInsertBefore);

    RecalcLayout();
}

//=============================================================================
/**
 * Remove a child from this control.
 * Overrides to make the layout get recalculated as well.
 * @param inChild the child to be removed.
 * @see CControl::RemoveChild.
 */
void CListLayout::RemoveChild(CControl *inChild)
{
    CControl::RemoveChild(inChild);
    RecalcLayout();
}

//=============================================================================
/**
 * Set the size of this control.
 * Overrides CControl::SetSize to propagate down to children too.
 * @param inSize the new size.
 */
void CListLayout::SetSize(CPt inSize)
{
    if (inSize.x != GetSize().x) {
        m_IsResizing = true;

        // Go through all the children and set their sizes too
        ControlGraph::SIterator theChildren = GetChildren();
        for (; theChildren.HasNext(); ++theChildren) {
            std::shared_ptr<CControlData> theChild = (*theChildren);
            if (theChild->IsVisible()) {
                // Only care about the width, keep the height the same.
                theChild->SetSize(inSize.x, theChild->GetSize().y);
            }
        }
        m_IsResizing = false;
    }

    CControl::SetSize(inSize);
}

//=============================================================================
/**
 * Recalculate the layout of all the sub controls.
 * This will resize this control appropriately and reposition all the children.
 */
void CListLayout::RecalcLayout()
{
    m_IsResizing = true;

    long theHeight = 0;
    CPt theCurrentPos(0, 0);
    long theMinLen = 0;

    ControlGraph::SIterator theChildren = GetChildren();
    if (m_AlignChildrenLength) {
        // Go through the first time to determine what is the minimum length for the children
        for (; theChildren.HasNext(); ++theChildren) {
            std::shared_ptr<CControlData> theChild = (*theChildren);
            // Only handle visible children
            if (theChild->IsVisible()) {
                CPt theMinimumSize(theChild->GetMinimumSize());
                if (theMinimumSize.x > theMinLen)
                    theMinLen = theMinimumSize.x;
            }
        }
        // reset this
        theChildren = GetChildren();
    }

    // Go through all the children and set their position, as well as the minimum length if required
    // to
    for (; theChildren.HasNext(); ++theChildren) {
        std::shared_ptr<CControlData> theChild = (*theChildren);
        // Only handle visible children
        if (theChild->IsVisible()) {
            CPt theChildSize = theChild->GetSize();
            theHeight += theChildSize.y;

            if (m_AlignChildrenLength)
                theChild->SetSize(CPt(theMinLen, theChildSize.y));
            else { // this would be the only loop to get the minimum length
                CPt theMinimumSize(theChild->GetMinimumSize());
                if (theMinimumSize.x > theMinLen)
                    theMinLen = theMinimumSize.x;
            }

            // Update the position
            theChild->SetPosition(theCurrentPos);
            theCurrentPos.y += theChildSize.y;
        }
    }

    SetMinimumSize(CPt(theMinLen, theHeight));
    SetMaximumSize(CPt(LONG_MAX, theHeight));

    SetSize(CPt(theMinLen, theHeight));

    m_IsResizing = false;
}

//=============================================================================
/**
 * Notification to this control that one of the child controls has changed size.
 * @param inControl the control that changed size.
 */
void CListLayout::OnChildSizeChanged(CControl *inControl)
{
    Q_UNUSED(inControl);

    // If we are not actively resizing then redo the layout.
    if (!m_IsResizing) {
        QT3DS_PROFILE(OnChildSizeChanged);

        RecalcLayout();
    }
}

#ifdef _DEBUG
bool CListLayout::OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags)
{
    return CControl::OnMouseDown(inPoint, inFlags);
}
#endif