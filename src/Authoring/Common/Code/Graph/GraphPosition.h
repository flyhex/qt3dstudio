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
#include <QtCore/qvariant.h>
#include <QDebug>

namespace Q3DStudio {
namespace Graph {
    using namespace std;

    struct SGraphPosition
    {
        struct SInvalid
        {
            SInvalid() {}
            ~SInvalid() {}
        };
        struct SBegin
        {
            SBegin() {}
            ~SBegin() {}
        };
        struct SEnd
        {
            SEnd() {}
            ~SEnd() {}
        };
        typedef QVariant TPositionType;

        enum Enum {
            Invalid,
            Begin,
            End,
            Index,
        };

        TPositionType m_Position;

        SGraphPosition() {}
        SGraphPosition(const SGraphPosition &inOther)
            : m_Position(inOther.m_Position)
        {
        }
        template <typename TDataType>
        SGraphPosition(const TDataType &dtype)
        {
            m_Position = QVariant::fromValue(dtype);
        }

        Enum GetType() const {
            Q_ASSERT((QMetaType::Type)m_Position.type() >= QMetaType::User ||
                     (QMetaType::Type)m_Position.type() == QMetaType::Long);
            if (Q_LIKELY(QMetaType::Long == m_Position.type()))
                return Index;
            else if (QString(m_Position.typeName()) ==
                     QStringLiteral("Q3DStudio::Graph::SGraphPosition::SBegin")) {
                return Begin;
            }
            else if (QString(m_Position.typeName()) ==
                     QStringLiteral("Q3DStudio::Graph::SGraphPosition::SEnd")) {
                return End;
            }
            else
                return Invalid;
        }
        bool operator==(const SGraphPosition &inOther)
        {
            if (GetType() != inOther.GetType())
                return false;
            if (GetType() == Index)
                return m_Position.toInt() == m_Position.toInt();
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
                return m_Position.toInt();
            default:;
            }
            Q_ASSERT(0);
            return 0;
        }
    };
}
}

Q_DECLARE_METATYPE(Q3DStudio::Graph::SGraphPosition::SBegin)
Q_DECLARE_METATYPE(Q3DStudio::Graph::SGraphPosition::SEnd)
Q_DECLARE_METATYPE(Q3DStudio::Graph::SGraphPosition::SInvalid)

#endif
