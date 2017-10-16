/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSDataRef.h"

namespace qt3ds {
namespace foundation {
    class IPerfTimer;
    class Mutex;
}
}
//==============================================================================
//	Namespace
//==============================================================================
namespace Q3DStudio {
using namespace qt3ds::foundation;
class CPresentation;
class CFlow;

//==============================================================================
/**
 *	Binary load / save
 */
class IBinarySerializer : public NVReleasable
{
    //==============================================================================
    //	Fields
    //==============================================================================
protected:
    //==============================================================================
    //	Methods
    //==============================================================================
public:
    // Binary save is taken care of duruing the saving of the flow assets.  Binary load,
    // however, can happen in a deferred fasion and so needs to be exported.
    virtual BOOL BinaryLoad(CPresentation *inPresentation, size_t inElementMemoryOffset,
                            qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8> inData,
                            qt3ds::foundation::NVDataRef<qt3ds::QT3DSU8> inStringTableData,
                            qt3ds::foundation::IPerfTimer &inPerfTimer) = 0;
    virtual void BinarySave(CPresentation *inPresentation) = 0;

public: // Creation function
    static IBinarySerializer &Create();
};

} // namespace Q3DStudio
