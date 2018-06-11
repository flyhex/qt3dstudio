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
#ifndef INCLUDED_TEST_CMD_UTILS_H
#define INCLUDED_TEST_CMD_UTILS_H 1

#pragma once

#include "Qt3DSDMHandles.h"
#include "Qt3DSCommonPrecompile.h"

class CAsset;
class CAsset;
class CNode;

//==============================================================================
// Templates
//==============================================================================

// A Fingerprint is list of string-based metrics and is used to aid QA comparison
typedef std::list<Q3DStudio::CString> TFingerprint;

//==============================================================================
// Functions
//==============================================================================

qt3dsdm::Qt3DSDMInstanceHandle TestCmdUtilsAddObject(CAsset *inParent, CAsset *inChild,
                                                  long inSlideIndex);

TFingerprint TestCmdUtilsMakeFingerprint(CNode *inNode);

TFingerprint TestCmdUtilsMakeFingerprint(CAsset *inAsset, long inSlideIndex);

Q3DStudio::CString TestCmdUtilsFlatten(const TFingerprint &inFingerprint);

bool TestCmdUtilsCompare(const TFingerprint &inFingerprint1, const TFingerprint &inFingerprint2);

// long round( float inX );

#endif // INCLUDED_TEST_UTILS_H
