/****************************************************************************
**
** Copyright (C) 1999-2004 NVIDIA Corporation.
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
#ifndef INCLUDED_STUDIOCLIPBOARD_H
#define INCLUDED_STUDIOCLIPBOARD_H 1

#include "Qt3DSFile.h"
#include "StudioObjectTypes.h"

class COleDataObject;

class CStudioClipboard
{
protected:
    CStudioClipboard(){}

public:
    virtual ~CStudioClipboard();

    static Qt3DSFile GetObjectFromClipboard(bool inIsInternal, qint64 &outContextData);
    static void CopyObjectToClipboard(Qt3DSFile &inFile, qint64 inContextData, bool inIsInternal,
                                      EStudioObjectType inObjectType);
    static bool CanPasteObject(EStudioObjectType inDestType, bool inIsInternal = false);
    static bool IsPastingAcrossInstances();

protected:
    static Qt3DSFile GetObjectFromInternalClipboard(qint64 &outContextData);
    static void CopyObjectToInternalClipboard(Qt3DSFile &inFile, qint64 inContextData);
    static Qt3DSFile GetObjectFromGlobalClipboard(qint64 &outContextData, const QString &inMimeType);
    static void CopyObjectToGlobalClipboard(Qt3DSFile &inFile, qint64 inContextData,
                                            const QString &inMimeType);

public:
    static QString GetTextFromClipboard();
    static void CopyTextToClipboard(const QString &);
    static bool CanPasteText();

    static void ClearClipboard(); ///< clears the global clipboard

    // Copy/paste Actions
    static Qt3DSFile GetActionFromClipboard();
    static void CopyActionToClipboard(Qt3DSFile &inFile);
    static bool CanPasteAction();

protected:
    static void GetObjectFromGlobalClipboard(const QString &inMimeType,
                                             Qt3DSFile *outFile = NULL, qint64 *outContextData = NULL);

private:
    static Qt3DSFile s_InternalClipObject; ///< the cache for internal clipboard operations
    static qint64 s_InternalContextData; ///< any arbitrary data for internal copy cmds.
    static EStudioObjectType s_AssetType; ///< asset type of the clipobject
    static qint64
        s_CopyProcessID; ///< process ID of this executable during the copy (for global copies only)
};

#endif // INCLUDED_STUDIOCLIPBOARD_H
