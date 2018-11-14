/****************************************************************************
**
** Copyright (C) 2005 NVIDIA Corporation.
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
#ifndef INCLUDED_PATHCONSTRUCTIONHELPER_H
#define INCLUDED_PATHCONSTRUCTIONHELPER_H 1

#include "Qt3DSDMHandles.h"
#include "Qt3DSString.h"

//==============================================================================
//	Forwards
//==============================================================================
#pragma once

class CDoc;

//==============================================================================
/**
 *	@class CPathConstructionHelper
 */
class CPathConstructionHelper
{
    //==============================================================================
    //	 Static Methods
    //==============================================================================
public:
    static QString BuildAbsoluteReferenceString(CDoc *inDoc,
                                                qt3dsdm::Qt3DSDMInstanceHandle inInstance);
    static QString
    BuildRelativeReferenceString(CDoc *inDoc, qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                 qt3dsdm::Qt3DSDMInstanceHandle inRootInstance);
    static QString BuildRelativeReferenceString(const QString &inAbsObjPath,
                                                const QString &inAbsRootPath);
    static QString EscapeAssetName(const QString &inAssetName);

    static const QString &GetThisString();
    static const QString &GetEscapeChar();
    static const QString &GetSceneString();
    static const QString &GetParentString();
    static const QString &GetPathDelimiter();
};

#endif // INCLUDED_PATHCONSTRUCTIONHELPER_H
