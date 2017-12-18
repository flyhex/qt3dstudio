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

#pragma once
#ifndef LAZYFLOWH
#define LAZYFLOWH
#include "Control.h"

#ifdef WIN32
#include <minmax.h>
#endif

namespace Q3DStudio {
namespace Control {

    // Specializers used to specialize the layout algorithm in the X or Y direction.
    // You could, in fact, specialzie the algorithm in any arbitrary direction including
    // 3d where components are sumed in the chosen direction and maxed in any other
    // orthogonal non-chosen directions.

    // Specializes the general layout algorithm in the x or y direction.
    struct ILazyFlowLayoutSpecializer
    {
    protected:
        virtual ~ILazyFlowLayoutSpecializer() {}

    public:
        static inline long SafeSum(long lhs, long rhs)
        {
            if (lhs == LONG_MAX || rhs == LONG_MAX)
                return LONG_MAX;
            return lhs + rhs;
        }
        // These functions allow us to apply the lazy flow layout algorithm to
        // either horizontal or vertical layouts.
        // Sum the variable in the direction we are going, and max any other variables
        CPt SumMax(const CPt &inCurrentSum, const CPt &inOther)
        {
            long curSum = GetSummedVariable(inCurrentSum);
            long nextSum = GetSummedVariable(inOther);
            long resultSum = SafeSum(curSum, nextSum);
            long resultMax = qMax(GetMaxedVariable(inCurrentSum), GetMaxedVariable(inOther));
            return ToPoint(resultSum, resultMax);
        }
        // Return the summed variable
        virtual long &GetSummedVariable(CPt &inPt) = 0;
        const long &GetSummedVariable(const CPt &inPt)
        {
            return GetSummedVariable(const_cast<CPt &>(inPt));
        }
        // return the maxed variable
        virtual long &GetMaxedVariable(CPt &inPt) = 0;
        const long &GetMaxedVariable(const CPt &inPt)
        {
            return GetMaxedVariable(const_cast<CPt &>(inPt));
        }
        // Create a new point from the combination of a summed variable and a maxed variable.
        virtual CPt ToPoint(long inSummedVariable, long inMaxedVariable) = 0;
    };

    struct SVerticalLayoutSpecializer : public ILazyFlowLayoutSpecializer
    {
        long &GetSummedVariable(CPt &inPt) override { return inPt.y; }
        long &GetMaxedVariable(CPt &inPt) override { return inPt.x; }
        CPt ToPoint(long inSummedVariable, long inMaxedVariable) override
        {
            return CPt(inMaxedVariable, inSummedVariable);
        }
    };

    struct SHorizontalLayoutSpecializer : public ILazyFlowLayoutSpecializer
    {
        long &GetSummedVariable(CPt &inPt) override { return inPt.x; }
        long &GetMaxedVariable(CPt &inPt) override { return inPt.y; }
        CPt ToPoint(long inSummedVariable, long inMaxedVariable) override
        {
            return CPt(inSummedVariable, inMaxedVariable);
        }
    };

    struct SSizeGroup
    {
        CPt m_Min;
        CPt m_Max;
        CPt m_Pref;
    };

    class CLazyFlowBase : public CControl
    {
        mutable bool m_MinMaxPreferredDirty;
        mutable bool m_CalculatingMinMaxPref;
        bool m_AutoMinMaxPref;
        CPt m_BeginMargin;
        CPt m_EndMargin;

    public:
        typedef vector<pair<std::shared_ptr<CControlData>, CPt>> TControlSizeList;

        CLazyFlowBase()
            : m_MinMaxPreferredDirty(true)
            , m_CalculatingMinMaxPref(false)
            , m_AutoMinMaxPref(true)
            , m_ChildGap(0)
        {
        }

        CPt GetMinimumSize() override;
        CPt GetPreferredSize() override;
        CPt GetMaximumSize() override;

        virtual void SetGapBetweenChildren(long inGap)
        {
            m_ChildGap = inGap;
            MarkChildrenNeedLayout();
        }
        virtual long GetGapBetweenChildren() const { return m_ChildGap; }

        // Also sets min max preferred dirty.
        void LayoutChildren() override;
        void MarkChildrenNeedLayout() override;
        void NotifyParentNeedsLayout() override;
        void SetLayout(CPt inSize, CPt inPosition) override;

        virtual void SetAutoMinMaxPref(bool inAuto) { m_AutoMinMaxPref = inAuto; }
        bool GetAutoMinMaxPref() const { return m_AutoMinMaxPref; }

        // Legacy with FlowLayout.
        virtual void SetAutoMin(bool inAuto) { m_AutoMinMaxPref = inAuto; }

        virtual void SetTopMargin(long inPixels)
        {
            m_BeginMargin.y = inPixels;
            MarkChildrenNeedLayout();
        }
        virtual long GetTopMargin() const { return m_BeginMargin.y; }

        virtual void SetLeftMargin(long inPixels)
        {
            m_BeginMargin.x = inPixels;
            MarkChildrenNeedLayout();
        }
        virtual long GetLeftMargin() const { return m_BeginMargin.x; }

        virtual void SetBottomMargin(long inPixels)
        {
            m_EndMargin.y = inPixels;
            MarkChildrenNeedLayout();
        }
        virtual long GetBottomMargin() const { return m_EndMargin.y; }

        virtual void SetRightMargin(long inPixels)
        {
            m_EndMargin.x = inPixels;
            MarkChildrenNeedLayout();
        }
        virtual long GetRightMargin() const { return m_EndMargin.x; }

    protected:
        void SetMinMaxPrefDirty() { m_MinMaxPreferredDirty = true; }
        virtual ILazyFlowLayoutSpecializer &GetSpecializer() = 0;

        long m_ChildGap;
        long GetGapTotal();
        // Margins are begin/end meaning before any children, after any children
        long GetBeginMargin() { return GetSpecializer().GetSummedVariable(m_BeginMargin); }
        long GetEndMargin() { return GetSpecializer().GetSummedVariable(m_EndMargin); }
        // And then margines that are beside the row of children, either before the row or after it.
        long GetBeforeRowMargin() { return GetSpecializer().GetMaxedVariable(m_BeginMargin); }
        long GetAfterRowMargin() { return GetSpecializer().GetMaxedVariable(m_EndMargin); }

        // I only implement these algorithms once, and I use a specializer to make them apply
        // to either a left-to-right or a top-to-bottom layout.
        void DistributeChildSizes(TControlSizeList &ioList);
        void PerformLayout(TControlSizeList &ioList);
        SSizeGroup CalcChildrenMinMaxPref();

        virtual SSizeGroup GetChildSizes(std::shared_ptr<CControlData> inControl);

        void RecalcMinMaxPref();
        void MaybeRecalcMinMaxPerf() const;
    };

    template <typename TSpecializerType>
    class TLazyFlowBase : public CLazyFlowBase
    {
        TSpecializerType m_Specializer;

    public:
        ILazyFlowLayoutSpecializer &GetSpecializer() override { return m_Specializer; }
    };

    struct SVerticalLazyFlow : public TLazyFlowBase<SVerticalLayoutSpecializer>
    {
    };

    struct SHorizontalLazyFlow : public TLazyFlowBase<SHorizontalLayoutSpecializer>
    {
    };

    class CRuntimeLazyFlow : public CLazyFlowBase
    {
    public:
        enum EFlowDirection {
            FLOW_VERTICAL,
            FLOW_HORIZONTAL,
        };

    private:
        SHorizontalLayoutSpecializer m_HorizontalSpecifier;
        SVerticalLayoutSpecializer m_VerticalSpecifier;
        EFlowDirection m_FlowDirection;

    public:
        CRuntimeLazyFlow()
            : m_FlowDirection(FLOW_VERTICAL)
        {
        }

        virtual void SetFlowDirection(EFlowDirection inFlowDirection)
        {
            m_FlowDirection = inFlowDirection;
            MarkChildrenNeedLayout();
        }

    protected:
        ILazyFlowLayoutSpecializer &GetSpecializer() override
        {
            if (m_FlowDirection == FLOW_VERTICAL)
                return m_VerticalSpecifier;
            return m_HorizontalSpecifier;
        }
    };
}
}

#endif
