/****************************************************************************
**
** Copyright (C) 1999-2003 NVIDIA Corporation.
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

#include "IDragable.h"
#include "FileDropSource.h"
#include "ExplorerFileDropSource.h"
#include "TimelineDropSource.h"
#include "BasicObjectDropSource.h"
#include "Views.h"
#include "MainFrm.h"
#include "TimelineWidget.h"

CDropSource::CDropSource(long inFlavor, unsigned long inSize)
    : m_Flavor(inFlavor)
    , m_Size(inSize)
{
    g_StudioApp.GetViews()->getMainFrame()->getTimelineWidget()->enableDnD();
}

CDropSource::~CDropSource()
{
    g_StudioApp.GetViews()->getMainFrame()->getTimelineWidget()->enableDnD(false);
}

// Create a drop source using a file path. Used for files
CDropSource *CDropSourceFactory::Create(long inFlavor, const QString &filePath)
{
    CDropSource *theDropSource(nullptr);
    switch (inFlavor) {
    case QT3DS_FLAVOR_FILE:
        theDropSource = new CExplorerFileDropSource(inFlavor, filePath);
        break;

    case QT3DS_FLAVOR_ASSET_UICFILE:
        theDropSource = new CFileDropSource(inFlavor, filePath);
        break;

    default:
        break;
    }

    return theDropSource;
}

// Create a drop source using a draggable. Used for anything that implements IDragable
CDropSource *CDropSourceFactory::Create(long inFlavor, IDragable *inDragable)
{
    CDropSource *theDropSource(nullptr);
    switch (inFlavor) {
    case QT3DS_FLAVOR_BASIC_OBJECTS:
        theDropSource = new CBasicObjectDropSource(inFlavor, inDragable);
        break;

    case QT3DS_FLAVOR_ASSET_TL:
        theDropSource = new CTimeLineDropSource(inFlavor, inDragable);
        break;

    default:
        // unsupported flavor
        break;
    }

    return theDropSource;
}
