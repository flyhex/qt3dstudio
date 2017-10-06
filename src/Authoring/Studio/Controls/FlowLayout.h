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
#ifndef INCLUDED_FLOW_LAYOUT_H
#define INCLUDED_FLOW_LAYOUT_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "Control.h"
#include "BlankControl.h"

//==============================================================================
//	Classes
//==============================================================================

//==============================================================================
/**
*	@class	CFlowLayout
*	@brief	CHRIS.EDWARDS needs to enter a brief description here.
*
*	CHRIS.EDWARDS needs to enter a long description here.
*/
class CFlowLayout : public CControl
{
public:
    enum EFlowDirection {
        FLOW_VERTICAL,
        FLOW_HORIZONTAL,
    };

    enum EVerticalAlignment {
        ALIGN_TOP,
        ALIGN_BOTTOM,
        ALIGN_CENTER,
        ALIGN_VERT_NEITHER,
    };

    enum EHorizontalAlignment {
        ALIGN_LEFT,
        ALIGN_RIGHT,
        ALIGN_MIDDLE,
        ALIGN_HORIZ_NEITHER,
    };

    CFlowLayout(CControl *inControl = nullptr, bool inUseControl = true);
    virtual ~CFlowLayout();

    DEFINE_OBJECT_COUNTER(CFlowLayout)

    void SetSize(CPt inSize) override;
    void SetLayout(CPt inSize, CPt inPosition) override;
    void OnParentChanged(CControl *inParent) override;

    void SetAlignment(EVerticalAlignment inVertical, EHorizontalAlignment inHorizontal);
    long SetLeftMargin(long inMargin);
    long GetLeftMargin() const;
    long SetRightMargin(long inMargin);
    long GetRightMargin() const;
    long SetTopMargin(long inMargin);
    long GetTopMargin() const;
    long SetBottomMargin(long inMargin);
    long GetBottomMargin() const;
    long SetGapBetweenChildren(long inGap);
    void SetDebug(bool inFlag) { m_DebugFlag = inFlag; }
    void SetAutoMin(bool inFlag) { m_AutoMin = inFlag; }

    virtual void SetFlowDirection(EFlowDirection inFlowDirection);
    void AddChild(CControl *inControl, CControl *inInsertBefore = nullptr) override;
    void RemoveChild(CControl *inControl) override;
    void Draw(CRenderer *inRenderer) override;
    virtual void DisableBlank();
    void OnChildSizeChanged(CControl *inControl) override;

    virtual void RecalcLayout();
    void SuspendRecalcLayout(bool inSuspendFlag);
    void ResetMinMaxPref() override;
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    bool OnMouseRDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;

protected:
    CPt GetTotalPreferredSizes();
    void ResizeExpandableControlsY(long inExtraSpace, long inNumExpandable);
    void ResizeExpandableControlsX(long inExtraSpace, long inNumExpandable);
    void ResetChildPositions();
    void DistributeRemaining(long inExtraSpace);
    void RecalcChild(CControl *inChild, CPt &ioPosition, long &ioNumExpandableX,
                     long &ioNumExpandableY);

    bool m_HasUnusedSpace;
    bool m_BlankDisable;
    EFlowDirection m_FlowDirection;
    EVerticalAlignment m_VerticalAlignment;
    EHorizontalAlignment m_HorizontalAlignment;
    bool m_ResizingChildren;
    long m_LeftMargin; ///< gap at left of this control
    long m_RightMargin; ///< gap at right of this control
    long m_TopMargin; ///< gap at top of this control
    long m_BottomMargin; ///< gap at bottom of this control
    long m_ChildGap; ///< gap between each child control when laying out everything
    bool m_DebugFlag;
    bool m_AutoMin;

    bool m_SuspendRecalcLayout; ///< flag to suspend recalculation of the layout; slow down is
                                ///noticeable if this control has many children

private:
    CControl *m_BlankControl; ///< used when all items are at their max and there si still space
};

#endif // INCLUDED_FLOW_LAYOUT_H
