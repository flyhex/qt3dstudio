/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#include "Qt3DSDMMaterialInspectable.h"
#include "Qt3DSDMInspectorGroup.h"
#include "Qt3DSDMInspectorRow.h"
#include "Core.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMHandles.h"
#include "Doc.h"
#include "GenericFunctor.h"
#include "StudioApp.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "IDocumentReader.h"
#include "Dispatch.h"
#include "IDirectoryWatchingSystem.h"
#include "Qt3DSDMSignals.h"
#include "Qt3DSString.h"

using namespace qt3dsdm;

Qt3DSDMMaterialInspectorGroup::Qt3DSDMMaterialInspectorGroup(
        CStudioApp &inApp,
        const QString &inName,
        Qt3DSDMInspectable &inInspectable,
        long inIndex)
    : Qt3DSDMInspectorGroup(inApp, inName, inInspectable, inIndex)
{
}

struct SQt3DSDMMaterialInspectorGroup : public Qt3DSDMMaterialInspectorGroup
{
    SQt3DSDMMaterialInspectorGroup(CStudioApp &inApp, const QString &inName,
                                   Qt3DSDMInspectable &inInspectable, long inIndex)
        : Qt3DSDMMaterialInspectorGroup(inApp, inName, inInspectable, inIndex)
    {
        QString theMaterialGroupName = QStringLiteral("Material");
        m_isMaterialGroup = (inName == theMaterialGroupName);
    }

    bool isMaterialGroup() const override
    {
        return m_isMaterialGroup;
    }

private:
    bool m_isMaterialGroup;
};

CInspectorGroup *Qt3DSDMMaterialInspectable::GetGroup(long inIndex)
{
    QString theGroupName = GetGroupName(inIndex);

    Qt3DSDMInspectorGroup *theGroup =
            new SQt3DSDMMaterialInspectorGroup(m_App, theGroupName, *this, inIndex);

    TMetaDataPropertyHandleList theProperties = GetGroupProperties(inIndex);
    size_t thePropertyCount = theProperties.size();

    for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount; ++thePropertyIndex)
        theGroup->CreateRow(m_Core->GetDoc(), theProperties[thePropertyIndex]);

    return theGroup;
}
