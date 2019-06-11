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
#ifndef GUIDEINSPECTABLE_H
#define GUIDEINSPECTABLE_H

#include "Qt3DSDMHandles.h"
#include "InspectableBase.h"
#include "IDocumentEditor.h"
#include "IInspectableItem.h"

class GuideInspectable : public CInspectableBase
{
public:
    GuideInspectable(qt3dsdm::Qt3DSDMGuideHandle inGuide);

    Q3DStudio::IDocumentReader &Reader() const;
    // Interface
    EStudioObjectType getObjectType() const override;
    Q3DStudio::CString getName() override;
    long getGroupCount() const override;
    CInspectorGroup *getGroup(long) override;
    bool isValid() const override;
    bool isMaster() const override;
    qt3dsdm::Qt3DSDMInstanceHandle getInstance() const override;

    // Implementation to get/set properties

    void SetDirection(const qt3dsdm::SValue &inValue);
    void SetPosition(const qt3dsdm::SValue &inValue);
    void SetWidth(const qt3dsdm::SValue &inValue);

    qt3dsdm::SValue GetDirection();
    qt3dsdm::SValue GetPosition();
    qt3dsdm::SValue GetWidth();

    Q3DStudio::IDocumentEditor &Editor();
    void Commit();
    void Rollback();
    void FireRefresh();
    void Destroy();

    bool isHorizontal() const;

    const std::vector<std::shared_ptr<IInspectableAttributeItem>> &properties() const;

private:
    qt3dsdm::Qt3DSDMGuideHandle m_Guide;
    Q3DStudio::CUpdateableDocumentEditor m_Editor;
    std::vector<std::shared_ptr<IInspectableAttributeItem>> m_Properties;
};

#endif
