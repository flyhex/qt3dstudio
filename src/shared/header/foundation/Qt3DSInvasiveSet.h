/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT3DS_FOUNDATION_INVASIVE_SET_H
#define QT3DS_FOUNDATION_INVASIVE_SET_H
#include "Qt3DS.h"
#include "Qt3DSAllocator.h"
#include "Qt3DSAllocatorCallback.h"
#include "Qt3DSContainers.h"
#include <QtCore/qvector.h>

namespace qt3ds {
namespace foundation {

    template <typename TObjectType, typename TGetSetIndexOp, typename TSetSetIndexOp>
    class InvasiveSet
    {
        QVector<TObjectType *> m_set;

        InvasiveSet(const InvasiveSet &other);
        InvasiveSet &operator=(const InvasiveSet &other);

    public:
        InvasiveSet()
        {
        }

        bool insert(TObjectType &inObject)
        {
            QT3DSU32 currentIdx = TGetSetIndexOp()(inObject);
            if (currentIdx == QT3DS_MAX_U32) {
                TSetSetIndexOp()(inObject, m_set.size());
                m_set.push_back(&inObject);
                return true;
            }
            return false;
        }

        bool remove(TObjectType &inObject)
        {
            QT3DSU32 currentIdx = TGetSetIndexOp()(inObject);
            if (currentIdx != QT3DS_MAX_U32) {
                TObjectType *theEnd = m_set.back();
                TObjectType *theObj = &inObject;
                if (theEnd != theObj) {
                    TSetSetIndexOp()(*theEnd, currentIdx);
                    m_set[currentIdx] = theEnd;
                }
                m_set.pop_back();
                TSetSetIndexOp()(inObject, QT3DS_MAX_U32);
                return true;
            }
            return false;
        }

        bool contains(TObjectType &inObject) { return TGetSetIndexOp()(inObject) != QT3DS_MAX_U32; }

        void clear()
        {
            for (QT3DSU32 idx = 0; idx < m_set.size(); ++idx)
                TSetSetIndexOp()(*(m_set[idx]), QT3DS_MAX_U32);
            m_set.clear();
        }

        TObjectType *operator[](QT3DSU32 idx) { return m_set[idx]; }
        const TObjectType *operator[](QT3DSU32 idx) const { return m_set[idx]; }
        QT3DSU32 size() const { return m_set.size(); }
        TObjectType **begin() { return m_set.begin(); }
        TObjectType **end() { return m_set.end(); }
        const TObjectType **begin() const { return m_set.begin(); }
        const TObjectType **end() const { return m_set.end(); }
        const TObjectType *back() const { return m_set.back(); }
        TObjectType *back() { return m_set.back(); }
    };
}
}
#endif
