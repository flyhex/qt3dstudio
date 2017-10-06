/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef __TYPESERRORIDS_H__
#define __TYPESERRORIDS_H__

//==============================================================================
//	Includes
//==============================================================================
#include "ClientErrorIDs.h"

//==============================================================================
//	HRESULTS
//==============================================================================
#define TYPES_ERROR(inID, inName)                                                                  \
    const HRESULT inName = MAKE_HRESULT(LEVEL_ERROR, FACILITY_ITF, TYPES_START_ERROR_RANGE + inID);

TYPES_ERROR(0, TYPES_E_FAIL)
TYPES_ERROR(1, TYPES_E_COLOR_FAIL)
TYPES_ERROR(2, TYPES_E_MATRIX_FAIL)
TYPES_ERROR(3, TYPES_E_PLANE_FAIL)
TYPES_ERROR(4, TYPES_E_QUATERNION_FAIL)
TYPES_ERROR(5, TYPES_E_ROTATION3_FAIL)
TYPES_ERROR(6, TYPES_E_VECTOR2_FAIL)
TYPES_ERROR(7, TYPES_E_VECTOR3_FAIL)
TYPES_ERROR(8, TYPES_E_VECTOR4_FAIL)
TYPES_ERROR(9, TYPES_E_TRAVERSALSTACK_FAIL)

#endif // #ifndef __TYPESERRORIDS_H__