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
#include "stdafx.h"
#include "LazyFlow.h"
#include "ControlData.h"

using namespace Q3DStudio::Control;

template <typename TIteratorType>
struct SVisibleFilteredIterator
{
    TIteratorType m_BaseIterator;
    SVisibleFilteredIterator(TIteratorType inIter)
        : m_BaseIterator(inIter)
    {
        FindNextValidItem();
    }
    void FindNextValidItem()
    {
        while (m_BaseIterator.IsDone() == false
               && m_BaseIterator.GetCurrent()->IsVisible() == false)
            ++m_BaseIterator;
    }

    SVisibleFilteredIterator<TIteratorType> &operator++()
    {
        ++m_BaseIterator;
        FindNextValidItem();
        return *this;
    }
    std::shared_ptr<CControlData> operator*() { return *m_BaseIterator; }
    bool IsDone() { return m_BaseIterator.IsDone(); }
};

struct SaneMinMaxPref : public SSizeGroup
{
    SaneMinMaxPref(const SSizeGroup &inData)
        : SSizeGroup(inData)
    {
        // ensure max and pref are greater than or equal to minimum
        m_Pref.x = max(m_Pref.x, m_Min.x);
        m_Pref.y = max(m_Pref.y, m_Min.y);
        m_Max.x = max(m_Max.x, m_Min.x);
        m_Max.y = max(m_Max.y, m_Min.y);

        // Ensure the maximum has valid information
        if (m_Max.x == 0)
            m_Max.x = m_Pref.x;
        if (m_Max.y == 0)
            m_Max.y = m_Pref.y;

        // Ensure preference is no greater than the maximum
        m_Pref.x = min(m_Pref.x, m_Max.x);
        m_Pref.y = min(m_Pref.y, m_Max.y);
    }
};

CPt CLazyFlowBase::GetMinimumSize()
{
    MaybeRecalcMinMaxPerf();
    return CControl::GetMinimumSize();
}
CPt CLazyFlowBase::GetPreferredSize()
{
    MaybeRecalcMinMaxPerf();
    return CControl::GetPreferredSize();
}
CPt CLazyFlowBase::GetMaximumSize()
{
    MaybeRecalcMinMaxPerf();
    return CControl::GetMaximumSize();
}

void CLazyFlowBase::LayoutChildren()
{
    TControlSizeList theSizeList;
    DistributeChildSizes(theSizeList);
    PerformLayout(theSizeList);
    Invalidate();
}

void CLazyFlowBase::MarkChildrenNeedLayout()
{
    m_MinMaxPreferredDirty = true;
    CControl::MarkChildrenNeedLayout();
}

void CLazyFlowBase::NotifyParentNeedsLayout()
{
    if (m_CalculatingMinMaxPref == false)
        CControl::NotifyParentNeedsLayout();
}

void CLazyFlowBase::SetLayout(CPt inSize, CPt inPosition)
{
    bool wasDirty = m_MinMaxPreferredDirty;
    CControl::SetLayout(inSize, inPosition);
    m_MinMaxPreferredDirty = wasDirty;
}

long CLazyFlowBase::GetGapTotal()
{
    long theNumVisibleChildren = 0;

    // Ensure we don't count any gaps for children that are invisible
    for (SVisibleFilteredIterator<ControlGraph::SIterator> theIter = GetChildren();
         theIter.IsDone() == false; ++theIter, ++theNumVisibleChildren) {
    }

    if (theNumVisibleChildren > 0)
        return m_ChildGap * theNumVisibleChildren;

    return 0;
}

void CLazyFlowBase::DistributeChildSizes(TControlSizeList &ioList)
{
    ILazyFlowLayoutSpecializer &theSpecializer(GetSpecializer());
    SSizeGroup theChildSizes(CalcChildrenMinMaxPref());
    CPt theMin(theChildSizes.m_Min);
    CPt thePref(theChildSizes.m_Pref);

    CPt theSize = GetSize();
    long &theSizeSum(theSpecializer.GetSummedVariable(theSize));
    long &theSizeMax = theSpecializer.GetMaxedVariable(theSize);

    long theTotalInlineMargin =
        ILazyFlowLayoutSpecializer::SafeSum(GetBeginMargin(), GetEndMargin());
    // long theTotalBesideMargin = ILazyFlowLayoutSpecializer::SafeSum( GetBeforeRowMargin(),
    // GetAfterRowMargin() );
    long theGapTotal(GetGapTotal());
    // Remove child independent size information.
    theSizeSum = theSizeSum - theGapTotal - theTotalInlineMargin;
    theSizeMax = theSizeMax - GetBeforeRowMargin() - GetAfterRowMargin();

    // sanitize the size after we remove the child indepedent size information
    theSize.x = max(theMin.x, theSize.x);
    theSize.y = max(theMin.y, theSize.y);

    // If we have more space than we can use.
    if (theSizeSum >= theSpecializer.GetSummedVariable(thePref)) {
        vector<long> theResizeableChildren;
        long theIndex = 0;
        CPt theLayoutRect;
        for (SVisibleFilteredIterator<ControlGraph::SIterator> theIter = GetChildren();
             theIter.IsDone() == false; ++theIter, ++theIndex) {
            std::shared_ptr<CControlData> theChildControl(*theIter);
            SaneMinMaxPref saneSizes(GetChildSizes(*theIter));
            long theSaneMax(theSpecializer.GetMaxedVariable(saneSizes.m_Max));
            CPt newSize(theSpecializer.ToPoint(theSpecializer.GetSummedVariable(saneSizes.m_Pref),
                                               min(theSizeMax, theSaneMax)));
            theLayoutRect = theSpecializer.SumMax(theLayoutRect, newSize);
            ioList.push_back(make_pair(theChildControl, newSize));
            if (theSpecializer.GetSummedVariable(saneSizes.m_Max) == LONG_MAX)
                theResizeableChildren.push_back(theIndex);
        }

        long theRectSum = theSpecializer.GetSummedVariable(theLayoutRect);
        if (theResizeableChildren.size() && theSizeSum > theRectSum) {
            long extra = (theSizeSum - theRectSum) / (long)theResizeableChildren.size();
            for (vector<long>::iterator theIter = theResizeableChildren.begin(),
                                        theEnd = theResizeableChildren.end();
                 theIter != theEnd; ++theIter) {
                pair<std::shared_ptr<CControlData>, CPt> &theChildAndSize(ioList[*theIter]);
                CPt theNewSize = theChildAndSize.second;
                theNewSize =
                    theSpecializer.ToPoint(theSpecializer.GetSummedVariable(theNewSize) + extra,
                                           theSpecializer.GetMaxedVariable(theNewSize));
                theChildAndSize.second = theNewSize;
            }
        }
    } else {
        long theMinSum = theSpecializer.GetSummedVariable(theMin);
        long thePrefSum = theSpecializer.GetSummedVariable(thePref);
        long minSizeX = max(theMinSum, theSizeSum);
        // We know that we got less than the pref size.
        long requestRange = thePrefSum - theMinSum;
        long actualRange = minSizeX - theMinSum;
        float ratio = 0.0f;
        if (requestRange != 0)
            ratio = (float)actualRange / (float)requestRange;

        // so we distribute the remaining to each object based on the ratio between
        // its preferred size to its minimums size, thus reducing each object equally
        // but on terms of the ratio between it's preferred size and minimum size.
        for (SVisibleFilteredIterator<ControlGraph::SIterator> theIter = GetChildren();
             theIter.IsDone() == false; ++theIter) {
            SaneMinMaxPref saneSizes(GetChildSizes(*theIter));
            std::shared_ptr<CControlData> theChildControl(*theIter);
            CPt childPref(saneSizes.m_Pref);
            CPt childMin(saneSizes.m_Min);
            long range = theSpecializer.GetSummedVariable(childPref)
                - theSpecializer.GetSummedVariable(childMin);
            long newRange = (long)((float)range * ratio + .5f);
            long newMin = theSpecializer.GetSummedVariable(childMin);
            CPt newSize = theSpecializer.ToPoint(newMin + newRange, theSizeMax);
            ioList.push_back(make_pair(theChildControl, newSize));
        }
    }
}

void CLazyFlowBase::PerformLayout(TControlSizeList &ioList)
{
    ILazyFlowLayoutSpecializer &theSpecializer(GetSpecializer());
    CPt thePos(theSpecializer.ToPoint(GetBeginMargin(), GetBeforeRowMargin()));
    long &theSummedVar = theSpecializer.GetSummedVariable(thePos);

    for (TControlSizeList::iterator iter = ioList.begin(), end = ioList.end(); iter != end;
         ++iter) {
        iter->first->SetLayout(iter->second, thePos);
        // Bump the position out to the next object
        theSummedVar += theSpecializer.GetSummedVariable(iter->second) + m_ChildGap;
    }
}

// Ignoring margins, calculate the min,max, and preferred sizes of the aggregate of our children
SSizeGroup CLazyFlowBase::CalcChildrenMinMaxPref()
{
    ILazyFlowLayoutSpecializer &theSpecializer(GetSpecializer());
    SSizeGroup retval;
    for (SVisibleFilteredIterator<ControlGraph::SIterator> theIter = GetChildren();
         theIter.IsDone() == false; ++theIter) {
        SaneMinMaxPref saneSizes(GetChildSizes(*theIter));
        retval.m_Min = theSpecializer.SumMax(retval.m_Min, saneSizes.m_Min);
        retval.m_Max = theSpecializer.SumMax(retval.m_Max, saneSizes.m_Max);
        retval.m_Pref = theSpecializer.SumMax(retval.m_Pref, saneSizes.m_Pref);
    }
    return retval;
}

void CLazyFlowBase::RecalcMinMaxPref()
{
    ILazyFlowLayoutSpecializer &theSpecializer(GetSpecializer());
    // Vertical means that we take the max of the x widths
    // and sum the y heights;
    // ensuring that LONG_MAX always sums to LONG_MAX
    long theRowMargin =
        ILazyFlowLayoutSpecializer::SafeSum(GetBeforeRowMargin(), GetAfterRowMargin());
    CPt theMin(theSpecializer.ToPoint(GetBeginMargin() + GetEndMargin(), theRowMargin));
    CPt theMax(theMin);
    CPt thePref(theMin);
    SSizeGroup theChildSizes(CalcChildrenMinMaxPref());
    theMin = theSpecializer.SumMax(theChildSizes.m_Min, theMin);
    theMax = theSpecializer.SumMax(theChildSizes.m_Max, theMax);
    thePref = theSpecializer.SumMax(theChildSizes.m_Pref, thePref);

    long theGapTotal(GetGapTotal());
    theMin = theSpecializer.ToPoint(
        ILazyFlowLayoutSpecializer::SafeSum(theSpecializer.GetSummedVariable(theMin), theGapTotal),
        ILazyFlowLayoutSpecializer::SafeSum(theSpecializer.GetMaxedVariable(theMin), theRowMargin));

    thePref = theSpecializer.ToPoint(
        ILazyFlowLayoutSpecializer::SafeSum(theSpecializer.GetSummedVariable(thePref), theGapTotal),
        ILazyFlowLayoutSpecializer::SafeSum(theSpecializer.GetMaxedVariable(thePref),
                                            theRowMargin));

    theMax = theSpecializer.ToPoint(
        ILazyFlowLayoutSpecializer::SafeSum(theSpecializer.GetSummedVariable(theMax), theGapTotal),
        ILazyFlowLayoutSpecializer::SafeSum(theSpecializer.GetMaxedVariable(theMax), theRowMargin));

    // Let's ensure we keep sane min,max,etc.
    thePref.x = max(thePref.x, theMin.x);
    thePref.y = max(thePref.y, theMin.y);
    theMax.x = max(theMax.x, theMin.x);
    theMax.y = max(theMax.y, theMin.y);

    SetMinimumSize(theMin);
    SetPreferredSize(thePref);
    SetMaximumSize(theMax);
}

void CLazyFlowBase::MaybeRecalcMinMaxPerf() const
{
    if (m_MinMaxPreferredDirty) {
        m_MinMaxPreferredDirty = false;
        if (m_AutoMinMaxPref) {
            m_CalculatingMinMaxPref = true;
            CLazyFlowBase &flow(const_cast<CLazyFlowBase &>(*this));
            flow.RecalcMinMaxPref();
            m_CalculatingMinMaxPref = false;
        }
    }
}

SSizeGroup CLazyFlowBase::GetChildSizes(std::shared_ptr<CControlData> inControl)
{
    SSizeGroup theGroup;
    theGroup.m_Min = inControl->GetMinimumSize();
    theGroup.m_Max = inControl->GetMaximumSize();
    theGroup.m_Pref = inControl->GetPreferredSize();
    return theGroup;
}
