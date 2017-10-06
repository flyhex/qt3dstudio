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
#ifndef GRAPHPOSITIONH
#define GRAPHPOSITIONH
#include <boost/variant.hpp>
#include <boost/operators.hpp>

namespace Q3DStudio {
namespace Graph {

    using namespace boost;
    using namespace std;

    struct SGraphPosition : public equality_comparable<SGraphPosition>
    {
        struct SInvalid
        {
            SInvalid() {}
        };
        struct SBegin
        {
            SBegin() {}
        };
        struct SEnd
        {
            SEnd() {}
        };
        typedef boost::variant<SEnd, SBegin, long, SInvalid> TPositionType;

        enum Enum {
            Invalid,
            Begin,
            End,
            Index,
        };

        struct SPosTypeVisitor : boost::static_visitor<Enum>
        {
            Enum operator()(SBegin) const { return Begin; }
            Enum operator()(SEnd) const { return End; }
            Enum operator()(SInvalid) const { return Invalid; }
            Enum operator()(long) const { return Index; }
        };

        TPositionType m_Position;

        SGraphPosition() {}
        SGraphPosition(const SGraphPosition &inOther)
            : m_Position(inOther.m_Position)
        {
        }
        template <typename TDataType>
        SGraphPosition(const TDataType &dtype)
            : m_Position(dtype)
        {
        }

        Enum GetType() const { return boost::apply_visitor(SPosTypeVisitor(), m_Position); }
        bool operator==(const SGraphPosition &inOther)
        {
            if (GetType() != inOther.GetType())
                return false;
            if (GetType() == Index)
                return get<long>(m_Position) == get<long>(inOther.m_Position);
            return true;
        }
        long GetIndex() const
        {
            switch (GetType()) {
            case Begin:
                return 0;
            case End:
                return LONG_MAX;
            case Index:
                return get<long>(m_Position);
            default:;
            }
            assert(0);
            return 0;
        }
    };
}
}

#endif
