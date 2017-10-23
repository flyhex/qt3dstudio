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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef __EXPLORERFILEDROPSOURCE_H__
#define __EXPLORERFILEDROPSOURCE_H__

//==============================================================================
//	Includes
//==============================================================================
#include "DropSource.h"
#include "Qt3DSFile.h"

class CStudioApp;
class CDropTarget;

//==============================================================================
/**
 *	@class	CExplorerFileDropSource
 *	@brief	Drop Source for QT3DS_FLAVOR_UICFILE
 *
 *	This class is meant to handle drag and drop for QT3DS_FLAVOR_UICFILE,
 *	for example when user drags a file from Explorer Window (outside Studio).
 */
class CExplorerFileDropSource : public CDropSource
{
protected:
    Qt3DSFile m_File;
    static bool s_FileHasValidTarget;

public:
    CExplorerFileDropSource(long inFlavor, void *inData, unsigned long inSize);

    bool CanMove() override;
    bool CanCopy() override;
    bool ValidateTarget(CDropTarget *inTarget) override;
    bool GetHasValidTarget();
    void SetHasValidTarget(bool inValid) override;
    Qt3DSFile GetFile() const { return m_File; }
};

#endif // #ifndef __EXPLORERFILEDROPSOURCE_H__
