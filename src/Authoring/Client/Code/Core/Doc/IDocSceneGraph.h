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
#ifndef IDOC_SCENE_GRAPH_H
#define IDOC_SCENE_GRAPH_H

#include "Qt3DSDMHandles.h"
#include "foundation/Qt3DSVec3.h"
#include "Pt.h"
#include <memory>

namespace qt3ds {
namespace foundation {
    class IStringTable;
}
}

#pragma once
namespace Q3DStudio {
using qt3ds::QT3DSVec3;
class Q3DSRenderBufferManager;
class ITextRenderer
{
public:
    void ReloadFonts();
};
class SGraphObject;
class IPathManager;

class IDocSceneGraph
{
protected:
    virtual ~IDocSceneGraph() {}
public:
    friend class std::shared_ptr<IDocSceneGraph>;

    virtual ITextRenderer *GetTextRenderer() = 0;
    virtual QT3DSVec3 GetIntendedPosition(qt3dsdm::Qt3DSDMInstanceHandle inHandle, CPt inPoint) = 0;
    virtual Q3DSRenderBufferManager *GetBufferManager() = 0;
    virtual IPathManager *GetPathManager() = 0;
    virtual qt3ds::foundation::IStringTable *GetRenderStringTable() = 0;
    // Request that this object renders.  May be ignored if a transaction
    // is ongoing so we don't get multiple rendering per transaction.
    virtual void RequestRender() = 0;
};
}
#endif
