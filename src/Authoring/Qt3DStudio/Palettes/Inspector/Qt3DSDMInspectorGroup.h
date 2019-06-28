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

#ifndef INCLUDED_QT3DSDM_INSPECTORGROUP_H
#define INCLUDED_QT3DSDM_INSPECTORGROUP_H

#include "InspectorGroup.h"
#include "Qt3DSDMHandles.h"

class Qt3DSDMInspectable;
class CDoc;

namespace Q3DStudio {
class Qt3DSDMInspectorRow;
};

class Qt3DSDMInspectorGroup : public CInspectorGroup
{
public:
    Qt3DSDMInspectorGroup(const QString &inName);
    ~Qt3DSDMInspectorGroup();

    void CreateRow(CDoc *inDoc, qt3dsdm::Qt3DSDMMetaDataPropertyHandle inProperty);

    const std::vector<Q3DStudio::Qt3DSDMInspectorRow *> &GetRows() const { return m_inspectorRows; }
    bool isMaterial() const { return m_name == QLatin1String("Material"); }

private:
    std::vector<Q3DStudio::Qt3DSDMInspectorRow *> m_inspectorRows;
};

#endif
