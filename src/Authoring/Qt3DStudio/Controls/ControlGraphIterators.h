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
#ifndef CONTROLGRAPHITERATORSH
#define CONTROLGRAPHITERATORSH
#include <vector>

namespace Q3DStudio {
namespace Control {
    class CControlData;
    using std::vector;

    namespace ControlGraph {
        struct SDummyGraphNode;

        struct SIteratorBase
        {
        protected:
            std::shared_ptr<CControlData> m_ControlData;
            vector<SDummyGraphNode *> *m_Children;
            long m_Index;

        public:
            SIteratorBase()
                : m_Children(nullptr)
                , m_Index(0)
            {
            }

            SIteratorBase(std::shared_ptr<CControlData> data,
                          vector<SDummyGraphNode *> &inChildren)
                : m_ControlData(data)
                , m_Children(&inChildren)
                , m_Index(0)
            {
            }

            SIteratorBase(const SIteratorBase &inOther)
                : m_ControlData(inOther.m_ControlData)
                , m_Children(inOther.m_Children)
                , m_Index(inOther.m_Index)
            {
            }

            SIteratorBase &operator=(const SIteratorBase &inOther)
            {
                if (this != &inOther) {
                    m_ControlData = inOther.m_ControlData;
                    m_Children = inOther.m_Children;
                    m_Index = inOther.m_Index;
                }
                return *this;
            }

            bool operator==(const SIteratorBase &other) { return m_Index == other.m_Index; }
            bool operator!=(const SIteratorBase &other) { return m_Index != other.m_Index; }

            std::shared_ptr<CControlData> GetCurrent();

            std::shared_ptr<CControlData> operator->() { return GetCurrent(); }
            std::shared_ptr<CControlData> operator*() { return GetCurrent(); }
        };

        struct SReverseIterator : SIteratorBase
        {
            SReverseIterator() {}
            SReverseIterator(std::shared_ptr<CControlData> data,
                             vector<SDummyGraphNode *> &inChildren)
                : SIteratorBase(data, inChildren)
            {
                m_Index = (long)inChildren.size() - 1;
            }
            SReverseIterator(const SReverseIterator &inOther)
                : SIteratorBase(inOther)
            {
            }
            SReverseIterator &operator=(const SReverseIterator &inOther)
            {
                SIteratorBase::operator=(inOther);
                return *this;
            }

            bool IsDone() { return m_Children == nullptr || m_Index < 0; }
            bool HasNext() { return m_Children && m_Index > -1; }
            SReverseIterator &operator++()
            {
                --m_Index;
                return *this;
            }
            SReverseIterator &operator+=(long inAmount)
            {
                m_Index -= inAmount;
                return *this;
            }
        };

        struct SIterator : SIteratorBase
        {
            SIterator() {}
            SIterator(std::shared_ptr<CControlData> data, vector<SDummyGraphNode *> &inChildren)
                : SIteratorBase(data, inChildren)
            {
            }
            SIterator(const SReverseIterator &inOther)
                : SIteratorBase(inOther)
            {
            }
            SIterator &operator=(const SIterator &inOther)
            {
                SIteratorBase::operator=(inOther);
                return *this;
            }

            bool IsDone() { return m_Children == nullptr || m_Index >= (long)m_Children->size(); }
            bool HasNext() { return m_Children && m_Index < (long)m_Children->size(); }
            SIterator operator++()
            {
                ++m_Index;
                return *this;
            }
            SIterator &operator+=(long inAmount)
            {
                m_Index += inAmount;
                return *this;
            }
        };
    }
}
}
#endif