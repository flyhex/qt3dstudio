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
#ifndef INSPECTABLEBASE_H
#define INSPECTABLEBASE_H

class CInspectorGroup;

#include "StudioObjectTypes.h"
#include "Qt3DSString.h"
#include "Qt3DSDMHandles.h"

 // Parent class of Inspectable types that will appear in the Inspector Palette.
class CInspectableBase
{
public:
    CInspectableBase() {}
    virtual ~CInspectableBase() {}

    virtual EStudioObjectType getObjectType() const = 0;
    virtual Q3DStudio::CString getName() = 0;
    virtual long getGroupCount() const = 0;
    virtual CInspectorGroup *getGroup(long inIndex) = 0;
    virtual bool isValid() const = 0;
    virtual bool isMaster() const = 0;
    virtual qt3dsdm::Qt3DSDMInstanceHandle getInstance() const = 0;
};

#endif // INSPECTABLEBASE_H
