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

#ifndef __FILEDROPSOURCE_H__
#define __FILEDROPSOURCE_H__

#include "DropSource.h"

class CDropTarget;

/**
 * @class CFileDropSource
 * @brief Drop Source for QT3DS_FLAVOR_ASSET_UICFILE
 *
 * This class is meant to handle drag and drop for QT3DS_FLAVOR_ASSET_UICFILE,
 * for example when user drags a file from within Studio.
 */
class CFileDropSource : public CDropSource
{
protected:
    QString m_FilePath;

    static bool s_FileHasValidTarget;

public:
    CFileDropSource(long inFlavor, const QString &filePath);

    bool CanMove() override;
    bool CanCopy() override;
    bool ValidateTarget(CDropTarget *inTarget) override;
    bool GetHasValidTarget();
    void SetHasValidTarget(bool inValid) override;

    CCmd *GenerateAssetCommand(qt3dsdm::Qt3DSDMInstanceHandle inTarget, EDROPDESTINATION inDestType,
                               qt3dsdm::Qt3DSDMSlideHandle inSlide) override;
};

#endif // #ifndef __FILEDROPSOURCE_H__
