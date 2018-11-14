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
#ifndef QT3DS_IMPORT_CONTAINERS_H
#define QT3DS_IMPORT_CONTAINERS_H

#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/vector.h"
#include "foundation/Qt3DSDataRef.h"
#include "Qt3DSDMWStrOps.h"

namespace qt3dsimp {

#define QT3DSIMP_FOREACH(idxnm, val)                                                                  \
    for (QT3DSU32 idxnm = 0, __numItems = (QT3DSU32)val; idxnm < __numItems; ++idxnm)

inline NVConstDataRef<wchar_t> toRef(const wchar_t *data, QT3DSU32 len = 0)
{
    if (IsTrivial(data))
        return NVConstDataRef<wchar_t>(L"", 0);
    len = len ? len : (QT3DSU32)wcslen(data);
    return NVConstDataRef<wchar_t>(data, len);
}
}

#endif
